/**
 * 重新思考几个问题：
 *
 * 1. shared_from_this 必要吗？
 *
 *    似乎是不必要的，因为：
 *    - 资源上的锁总保证其只会被单个线程占用，不存在多线程间析构的问题
 *    - 所以，资源的所有权要么是资源池，要么是单个线程，不存在多线程间共享的问题
 *
 *    如果不用智能指针呢？当线程移交资源到池时，谁持有资源的所有权？所以必须要用。
 *
 *    如果使用 unique_ptr 呢？那么如果资源要在多线程间共享，它岂不是会同时拥有
 *    shared_ptr 和 unique_ptr 两种所有权？这是肯定不对的。
 *    - 除非：unique_ptr<shared_ptr<T>>，这样就可以保证资源只有一个所有权
 *    - 得这么考虑：一般使用这个类的资源都不是线程间共享（所有权）的，所以不会有
 *      这个问题
 *
 *    主要是在 drop 操作中，由于要按序锁定，在从 here 找到 prev 时，需要放弃
 *    here 的锁， 而从锁被放弃到找到 prev 之间，prev 就可能被释放了，因此必须
 *    要有一种办法来保证 prev 到下次锁定前不会被释放，这就是 shared_from_this
 *    的作用。
 *
 *    结论：有必要，就是为了 drop 操作服务的
 *
 * 2. 要不要有哨兵？
 *
 *    哨兵的问题是：
 *    - 类型不安全，shared_ptr/unique_ptr 在转换时可能会发生切片
 *
 *    没有哨兵的问题是：
 *    - 当池中仅剩的一个资源被丢弃时，怎么更新资源池的头指针？
 *
 *    结论：有必要，否则无法实现资源池的头指针更新
 *
 *    那要不要有尾哨兵？
 *    - 有尾哨兵的问题是：尾哨兵的存在会对遍历带来问题，因为遍历器要持有节点上
 *      的锁 —— 那么 end() 返回的尾哨兵要不要持有锁呢？如果持有锁，那么多线程
 *      遍历就是不可能的：尾哨兵上会互斥。
 *
 *    结论：不能有尾哨兵，因为遍历器不能持有尾哨兵上的锁
 *
 * 3. 要不要用 virtual 析构函数？
 *
 *    设想这样一种情况：一个线程正在执行 drop，它通过 prev 给前一个结点刚刚
 *    创建了一个 shared_ptr<Node> 的指针，这个结点就被另一个线程 drop 掉了，
 *    但因为这个线程持有了 shared_ptr，所以这个结点没有在另一个线程 drop
 *    时被销毁，而是在 shared_ptr 被销毁时被销毁，这时结点会被切片（以 Node
 *    析构，而非 T），这问题就大了。
 *
 *    此外，virtual 析构函数可以避免模板导致的代码膨胀，因为大部分对象头处理
 *    的代码都可以放到内部命名空间中。
 *
 *    结论：要，因为有哨兵后可能会发生切片。
 *
 * 4. 要不要区分 next 和 prev 锁？
 *
 *    没必要，因为争用主要发生在申请资源时，仅仅隔离申请和释放不会减轻争用。
 *
 * 5. 因为有了析构函数，可以考虑搞成一个 RAII 的资源池，每个结点可以是不同类
 *    型的资源。需要增加额外的方法：取出下一个指定类型的资源。当然，可以再用
 *    模板提供一个类型安全的类，它使用静态类型转换避免运行时类型检查。
 */

#pragma once

#include "SpinMutex.hpp"
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>

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
  using SpinBit = SpinMutex::Bit<Pooled*, 0>;

private:
  std::shared_ptr<T> mNext{ nullptr };
  std::atomic<Pooled*> mPrev{ nullptr }; // 最低位用作自旋锁
  static_assert(alignof(std::atomic<T*>) >= 2);
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
  Iterator(std::shared_ptr<T> p) noexcept
    : std::shared_ptr<T>(std::move(p))
  {
    if (*this)
      SpinBit::lock(std::shared_ptr<T>::operator*().mPrev);
  }

  Iterator(const Iterator&) = delete;
  Iterator(Iterator&&) = default;
  Iterator& operator=(const Iterator&) = delete;
  Iterator& operator=(Iterator&&) = default;

  ~Iterator() noexcept
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
  if (next)
    SpinBit::lock(next->mPrev);
  std::shared_ptr<T>::operator=(std::move(next));
  SpinBit::unlock(t.mPrev);
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
  static bool is_in(const Pooled& t) noexcept
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
  static Iterator begin(T& t) noexcept { return { t.shared_from_this() }; }

  /**
   * @brief 遍历池中的剩余资源结束。
   */
  static Iterator end() noexcept { return {}; }

  /**
   * @brief 对池中的资源进行计数，这会锁定遍历 t 后的所有剩余资源。
   */
  static std::size_t count(T& t) noexcept;

public:
  /**
   * @see take(Pooled&)
   */
  std::shared_ptr<T> take() noexcept { return take(mStub); }

  /**
   * @see give(Pooled&, std::shared_ptr<T>)
   */
  void give(std::shared_ptr<T> r) noexcept { give(mStub, std::move(r)); }

  /**
   * @see begin(const Pooled&)
   */
  Iterator begin() noexcept
  {
    SpinBit::lock(mStub.mPrev);
    Iterator ret(mStub.mNext);
    SpinBit::unlock(mStub.mPrev);
    return ret;
  }

  /**
   * @see count(const Pooled&)
   */
  std::size_t count() noexcept
  {
    SpinBit::lock(mStub.mPrev);
    auto head = mStub.mNext;
    SpinBit::unlock(mStub.mPrev);
    return count(*head);
  }

private:
  Pooled mStub; // 不能这么干，在 shared_from_this 时会抛出 std::bad_weak_ptr
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
  hereBit.lock(); // 在锁定之后再断言
  assert(here.mNext == nullptr && hereBit.masked() == nullptr);

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

template<typename T>
std::size_t
Pooled<T>::Pool::count(T& t) noexcept
{
  std::size_t cnt = 0;
  for (auto it = begin(t), end = Pool::end(); it != end; ++it)
    ++cnt;
  return cnt;
}

} // namespace My
