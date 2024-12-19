#pragma once

#include "SpinMutex.hpp"
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>

namespace My {

namespace _Pooled {

class Iterator;
class Stub;

class Node : public std::enable_shared_from_this<Node>
{
  friend class Iterator;
  friend class Stub;

public:
  virtual ~Node() = default; // 为了在共享指针释放时区分节点和桩。

  // 所有方法都放到 Stub 类中，避免污染子类的命名空间。

private:
  // 所有节点和桩串成一个双向的链
  std::shared_ptr<Node> mNext{ nullptr }; // 共享指针是单向的，没有循环引用。
  std::atomic<std::uintptr_t> mPrev{ 0 }; // 最低位用作自旋锁。
};

static_assert(alignof(Node) >= 2);
using SpinBit = SpinMutex::Bit<std::uintptr_t, 0>;

class Iterator
{
public:
  Iterator() = default;

  /**
   * @param p 资源指针，必须未被锁定，遍历器构造完成后会被锁定。
   */
  Iterator(std::shared_ptr<Node> p) noexcept
    : mNode(std::move(p))
  {
    if (mNode)
      SpinBit::lock(mNode->mPrev);
  }

  Iterator(const Iterator&) = delete;
  Iterator(Iterator&& other)
    : mNode()
  {
    swap(*this, other);
  }

  Iterator& operator=(const Iterator&) = delete;
  Iterator& operator=(Iterator&& other)
  {
    swap(*this, other);
    return *this;
  }

  ~Iterator() noexcept
  {
    if (mNode)
      SpinBit::unlock(mNode->mPrev);
  }

  friend void swap(Iterator& lhs, Iterator& rhs) noexcept
  {
    using std::swap;
    swap(lhs.mNode, rhs.mNode);
  }

  operator bool() const noexcept { return bool(mNode); }
  Node* operator->() const noexcept { return mNode.get(); }
  Node& operator*() const noexcept { return *mNode; }
  Iterator& operator++() noexcept;

private:
  std::shared_ptr<Node> mNode;
};

class Stub : public Node
{
public:
  /**
   * @brief 检查 here 是否在池中。
   */
  static bool is_in(const Node& here) noexcept
  {
    return SpinBit::masked(here.mPrev);
  }

  /**
   * @brief 检查 here 是否被锁定。
   */
  static bool is_locked(const Node& here) noexcept
  {
    return SpinBit::locked(here.mPrev);
  }

  /**
   * @brief 从池中取出 after 之后的下一个资源，如果池空则返回空指针，
   * 返回的资源未被锁定。
   */
  static std::shared_ptr<Node> take(Node& after) noexcept;

  /**
   * @brief 从池中取出下一个指定类型的资源，如果没有则返回空指针，
   * 返回的资源未被锁定。
   */
  static std::shared_ptr<Node> take_if(Node& after,
                                       const std::type_info& type) noexcept;

  /**
   * @brief 向池中放入资源：将 here 插入到 after 之后，here 必须未被锁定。
   */
  static void give(Node& after, std::shared_ptr<Node> here) noexcept;

  /**
   * @brief 从池中丢弃一个资源 here，here 必须在池中且未被锁定。
   */
  static void drop(Node& node) noexcept;

  /**
   * @brief 丢弃 after 之后的所有资源。
   */
  static void clear(Node& after) noexcept;

  /**
   * @brief 从 node 开始遍历池中的剩余资源。
   */
  static Iterator begin(Node& node) noexcept
  {
    return { node.shared_from_this() };
  }

  /**
   * @brief 遍历池中的剩余资源结束。
   */
  static Iterator end() noexcept { return {}; }

  /**
   * @brief 统计 node 及之后的资源数量，这会锁定遍历 node 后的所有剩余资源。
   */
  static std::size_t count(Node& node) noexcept;

  /**
   * @brief 遍历桩之后的所有资源。
   */
  Iterator begin() noexcept
  {
    SpinBit::lock(mPrev);
    Iterator ret;
    if (mNext)
      ret = begin(*mNext);
    SpinBit::unlock(mPrev);
    return ret;
  }

  /**
   * @brief 计数桩之后的所有资源。
   */
  std::size_t count() noexcept
  {
    SpinBit::lock(mPrev);
    auto head = mNext;
    SpinBit::unlock(mPrev);
    if (!head)
      return 0;
    return count(*head);
  }
};

} // namespace _Pooled

/**
 * @brief 多线程资源池混入类：
 * 1. 允许多个线程并发地插入资源；
 * 2. 允许多个线程并发地申请任一资源，无论资源是哪个线程创建的；
 * 3. 允许遍历所有的资源对象，但不保证遍历时的一致性，例如可能
 *    遍历到最后一个资源对象时，第一个资源对象已经被销毁了。
 *
 * 示例：
 *
 * ```cpp
 * struct Resource : public Pooled<Resource> {};
 * Resource::Pool pool;
 * pool.give(std::make_shared<Resource>(r));
 * auto resource = pool.take();
 * Resource::Pool::drop(*resource);
 * ```
 *
 * @tparam T 资源类型。
 */
template<typename T>
struct Pooled : _Pooled::Node
{
  class Iterator;
  class Pool;

  std::shared_ptr<T> shared_from_this() noexcept
  {
    return std::reinterpret_pointer_cast<T>(_Pooled::Node::shared_from_this());
  }

  std::shared_ptr<const T> shared_from_this() const noexcept
  {
    return std::reinterpret_pointer_cast<const T>(
      _Pooled::Node::shared_from_this());
  }

  std::weak_ptr<T> weak_from_this() noexcept
  {
    return std::reinterpret_pointer_cast<T>(_Pooled::Node::shared_from_this());
  }

  std::weak_ptr<const T> weak_from_this() const noexcept
  {
    return std::reinterpret_pointer_cast<const T>(
      _Pooled::Node::shared_from_this());
  }
};

template<typename T>
class Pooled<T>::Iterator : public _Pooled::Iterator
{
public:
  T* operator->() const noexcept
  {
    return reinterpret_cast<T*>(_Pooled::Iterator::operator->());
  }

  T& operator*() const noexcept
  {
    return reinterpret_cast<T&>(_Pooled::Iterator::operator*());
  }

  Iterator& operator++() noexcept
  {
    return reinterpret_cast<Iterator&>(_Pooled::Iterator::operator++());
  }
};

/**
 * @brief 多线程资源池，线程安全。
 */
template<typename T>
class Pooled<T>::Pool
{
public:
  /**
   * @brief 检查资源是否在池中。
   *
   * @return true - 在池中；false - 不在池中。
   */
  static bool is_in(const T& t) noexcept { return _Pooled::Stub::is_in(t); }

  /**
   * @brief 基于一个池中节点（桩或资源）取出下一个资源，取出的资源会被移除出池。
   * 该方法是线程安全的，多个线程可以同时调用同一个对象上的该方法。
   *
   * @return 下一个资源（未被锁定），如果池空则返回空指针。
   */
  static std::shared_ptr<T> take(T& t) noexcept
  {
    return std::reinterpret_pointer_cast<T>(_Pooled::Stub::take(t));
  }

  /**
   * @brief 取出池中的下一个指定类型的资源，取出的资源会被移除出池。
   *
   * @return 下一个指定类型的资源（未被锁定），如果没有则返回空指针。
   */
  template<typename U>
  static std::shared_ptr<U> take_if(T& t) noexcept
  {
    return std::reinterpret_pointer_cast<U>(
      _Pooled::Stub::take_if(t, typeid(U)));
  }

  /**
   * @brief 归还资源 r，将 r 插入到 t 之后，r 必须不在池中且未被锁定。
   * 该方法对 r 线程不安全，在同一时刻最多只能有一个线程调用该方法。
   *
   * @param t 任何一个池中的节点。
   */
  static void give(T& t, std::shared_ptr<T> r) noexcept
  {
    _Pooled::Stub::give(t, std::reinterpret_pointer_cast<_Pooled::Node>(r));
  }

  /**
   * @brief 从池中移除一个资源 t，t 必须在池中且未被锁定。
   * 重复调用该方法时无操作。
   */
  static void drop(T& t) noexcept { _Pooled::Stub::drop(t); }

  /**
   * @brief 清空 t 之后的所有资源，t 必须未被锁定。
   */
  static void clear(T& t) noexcept { _Pooled::Stub::drop(t); }

  /**
   * @brief 从节点 t 开始遍历池中的剩余资源。
   */
  static Iterator begin(T& t) noexcept { return { t.shared_from_this() }; }

  /**
   * @brief 遍历池中的剩余资源结束。
   */
  static Iterator end() noexcept { return {}; }

  /**
   * @brief 对池中的资源进行计数，这会锁定遍历 t 后的所有剩余资源。
   */
  static std::size_t count(T& t) noexcept { return _Pooled::Stub::count(t); }

public:
  /**
   * @see take(Pooled&)
   */
  std::shared_ptr<T> take() noexcept
  {
    return std::reinterpret_pointer_cast<T>(_Pooled::Stub::take(*mStub));
  }

  /**
   * @see take_if(Pooled&)
   */
  template<typename U>
  std::shared_ptr<U> take_if() noexcept
  {
    return std::reinterpret_pointer_cast<U>(
      _Pooled::Stub::take_if(*mStub, typeid(U)));
  }

  /**
   * @see give(Pooled&, std::shared_ptr<T>)
   */
  void give(std::shared_ptr<T> r) noexcept
  {
    _Pooled::Stub::give(*mStub, std::move(r));
  }

  /**
   * @see drop(Pooled&)
   */
  void clear() noexcept { _Pooled::Stub::clear(*mStub); }

  /**
   * @see begin(const Pooled&)
   */
  Iterator begin() noexcept { return { mStub->begin() }; }

  /**
   * @see count(const Pooled&)
   */
  std::size_t count() noexcept { return mStub->count(); }

private:
  std::shared_ptr<_Pooled::Stub> mStub{ std::make_shared<_Pooled::Stub>() };
};

} // namespace My
