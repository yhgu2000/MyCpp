#pragma once

#include "SpinMutex.hpp"
#include <atomic>
#include <cassert>
#include <memory>

namespace My {

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
class Pooled : public std::enable_shared_from_this<T>
{
public:
  class Iterator;
  class Pool;
  using SpinBit = SpinMutex::Bit<T*, 0>;

  Pooled()
    : mNext(nullptr)
    , mPrev(nullptr)
  {
  }

private:
  std::shared_ptr<T> mNext;
  std::atomic<T*> mPrev; // 最低位用作自旋锁
  static_assert(alignof(std::atomic<T*>) >= 4);
};

/**
 * @brief 资源池遍历器，每个实例锁定一个资源并在析构时释放锁。
 */
template<typename T>
class Pooled<T>::Iterator : public std::shared_ptr<T>
{
public:
  Iterator() = default;

  /**
   * @param p 资源指针，必须未被锁定，遍历器构造完成后会被锁定。
   */
  Iterator(std::shared_ptr<T> p)
    : std::shared_ptr<T>(std::move(p))
  {
    SpinBit::lock(std::shared_ptr<T>::operator*().mPrev);
  }

  Iterator(const Iterator&) = delete;
  Iterator(Iterator&&) = default;
  Iterator& operator=(const Iterator&) = delete;
  Iterator& operator=(Iterator&&) = default;

  ~Iterator()
  {
    if (*this)
      SpinBit::unlock(std::shared_ptr<T>::operator*().mPrev);
  }

  Iterator& operator++() noexcept;
};

template<typename T>
typename Pooled<T>::Iterator&
Pooled<T>::Iterator::operator++() noexcept
{
  auto& t = std::shared_ptr<T>::operator*();
  auto next = t.mNext;
  if (!next) {
    std::shared_ptr<T>::reset();
    return *this;
  }
  SpinBit::lock(next->mPrev);
  SpinBit::unlock(t.mPrev);
  std::shared_ptr<T>::operator=(std::move(next));
  return *this;
}

/**
 * @brief 资源池，线程安全。
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
  static bool pooled(const Pooled& t) noexcept
  {
    return t.mPrev.load(std::memory_order_relaxed) != nullptr;
  }

  /**
   * @brief 基于一个池中节点（桩或资源）取出下一个资源，取出的资源会被移除出池。
   * 该方法是线程安全的，多个线程可以同时调用同一个对象上的该方法。
   *
   * @return 下一个资源（未被锁定），如果池空则返回空指针。
   */
  static std::shared_ptr<T> take(Pooled& t) noexcept;

  /**
   * @brief 归还资源 r，将 r 插入到 t 之后，r 必须不在池中且未被锁定。
   * 该方法对 r 线程不安全，在同一时刻最多只能有一个线程调用该方法。
   *
   * @param t 任何一个池中的节点。
   */
  static void give(Pooled& t, std::shared_ptr<T> r) noexcept;

  /**
   * @brief 从池中移除一个资源 t，t 必须在池中且未被锁定。
   * 重复调用该方法时无操作。
   */
  static void drop(T& t) noexcept;

  /**
   * @brief 从节点 t 开始遍历池中的剩余资源。
   */
  static Iterator begin(Pooled& t) { return { t.shared_from_this() }; }

  /**
   * @brief 遍历池中的剩余资源结束。
   */
  static Iterator end() { return {}; }

public:
  /**
   * @see take(Pooled&)
   */
  std::shared_ptr<T> take() { return take(mStub); }

  /**
   * @see give(Pooled&, std::shared_ptr<T>)
   */
  void give(std::shared_ptr<T> r) { give(mStub, std::move(r)); }

  /**
   * @see begin(const Pooled&)
   */
  Iterator begin() { return begin(mStub); }

private:
  Pooled mStub;
  // HACK 要这么改，否则类型不安全：
  // std::shared_ptr<T> mNext;
};

template<typename T>
std::shared_ptr<T>
Pooled<T>::Pool::take(Pooled& t) noexcept
{
  SpinBit prevBit(t.mPrev);
  prevBit.lock();
  auto here = t.mNext;
  if (!here) {
    prevBit.unlock();
    return nullptr;
  }

  SpinBit hereBit(here->mPrev);
  hereBit.lock();
  auto next = here->mNext;
  if (!next) {
    t.mNext = nullptr;
    prevBit.unlock(); // 尽可能早地释放锁
    hereBit.masked(nullptr);
    hereBit.unlock();
    return here;
  }

  SpinBit nextBit(next->mPrev);
  nextBit.lock();
  t.mNext = next;
  prevBit.unlock(); // 尽可能早地释放锁
  nextBit.masked(&t);
  nextBit.unlock(); // 尽可能早地释放锁

  here->mNext = nullptr;
  hereBit.masked(nullptr);
  hereBit.unlock();
  return here;
}

template<typename T>
void
Pooled<T>::Pool::give(Pooled& t, std::shared_ptr<T> r) noexcept
{
  assert(r != nullptr);
  T& here = *r;
  SpinBit hereBit(here.mPrev);
  hereBit.lock();
  assert(here.mNext == nullptr && here.mPrev == nullptr); // 在锁定之后再断言

  SpinBit prevBit(t.mPrev);
  prevBit.lock();
  auto next = t.mNext;
  if (!next) {
    t.mNext = std::move(r);
    prevBit.unlock(); // 尽可能早地释放锁
    hereBit.masked(&t);
    hereBit.unlock();
    return;
  }

  SpinBit nextBit(next->mPrev);
  nextBit.lock();
  t.mNext = std::move(r);
  prevBit.unlock(); // 尽可能早地释放锁
  hereBit.masked(&t);

  here.mNext = std::move(next);
  nextBit.masked(&here);
  hereBit.unlock();
  nextBit.unlock();
}

template<typename T>
void
Pooled<T>::Pool::drop(T& t) noexcept
{
  while (true) {
    SpinBit hereBit(t.mPrev);
    hereBit.lock();
    auto prev = hereBit.masked()->shared_from_this();
    assert(prev != nullptr); // 在锁定之后再断言
    hereBit.unlock();

    // 先释放 hereBit，等 prevBit 锁定后再重新锁定，避免死锁
    SpinBit prevBit(prev->mPrev);
    prevBit.lock();
    if (prev->mNext.get() != &t) {
      prevBit.unlock();
      continue; // 重试
    }
    hereBit.lock();

    auto next = t.mNext;
    if (!next) {
      prev->mNext = nullptr;
      prevBit.unlock(); // 尽可能早地释放锁
      hereBit.masked(nullptr);
      hereBit.unlock();
      return;
    }

    SpinBit nextBit(next->mPrev);
    nextBit.lock();
    prev->mNext = std::move(t.mNext);
    prevBit.unlock(); // 尽可能早地释放锁
    nextBit.masked(prev.get());
    nextBit.unlock(); // 尽可能早地释放锁

    t.mNext = nullptr;
    hereBit.masked(nullptr);
    hereBit.unlock();
    break;
  }
}

} // namespace My
