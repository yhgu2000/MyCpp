#pragma once

#include <atomic>
#include <cassert>

namespace My {

/**
 * @brief 池化混入类，用在并发编程中实现无锁的资源池。
 *
 * 示例：
 *
 * ```cpp
 * struct Resource : public Pooled<Resource> {};
 * Resource::Pool pool;
 * auto r = new Resource();
 * pool.give(r1);
 * r = pool.take();
 * ```
 *
 * @tparam T 资源类型。
 */

class Pooled;
using T = Pooled;
class Pooled
{
public:
  struct Pool;

  Pooled()
    : mNextPooled(nullptr)
    , mPrevPooled(nullptr)
  {
  }

  //* 在池中的对象通过 mPrevPooled 和 mNextPooled 两个指针形成一个双向链表。
  //*
  //* 定义一个对象 “在池中” 为：
  //*  1. 链表头 “在池中” ；
  //*  2. 如果一个对象被一个 “在池中” 的其它对象指向，则它也 “在池中” 。
  //*
  //* 要求每个方法在调用和返回时满足额外的不变量约束：
  //*  1. 任何一个不 “在池中” 的对象最多只能有一个线程访问它的头部。
  //*  2. 对于任何一个一直 “在池中” 的对象，如果它的 mNextPooled 在读取时不
  //*     为空，则在有限的步骤内 mNextPooled->mPrevPooled 一定会指向它自己。

  /**
   * @brief 检查当前资源是否在池中。
   *
   * @return true - 在池中；false - 不在池中。
   */
  // bool is_pooled() const
  // {
  //   auto prev = mPrevPooled.load(std::memory_order_acquire);
  //   return prev != nullptr;
  // }

  /**
   * @brief 基于一个已在池中的对象取出下一个对象。该方法是线程安全的，
   * 多个线程可以同时调用同一个对象上的该方法。
   *
   * @return T* 下一个资源，如果池空则返回 nullptr。
   */
  T* take_next()
  {
    while (true) {
      auto next = mNextPooled.load(std::memory_order_acquire);
      if (next == nullptr)
        return nullptr;
      // 因为当前对象是链表头或在池中，所以 next 也在池中，而首先要做的就是
      // 从池中取出 next
      auto next2 = next->mNextPooled.load(std::memory_order_acquire);
      if (!mNextPooled.compare_exchange_weak(
            next, next2, std::memory_order_release, std::memory_order_relaxed))
        continue;
      // 到这里，next 不在池中了，但因为 mNextPooled 仍然指向 next2，
      // 所以 next2 仍然在池中，所以要让 next2 满足不变量
      if (next2 != nullptr)
        next2->mPrevPooled.store(this, std::memory_order_release);
      // 由于 next 不在池中，所以怎么处理都随意了
      next->mPrevPooled.store(nullptr, std::memory_order_release);
      next->mNextPooled.store(nullptr, std::memory_order_release);
      return next;
    }
  }

  /**
   * @brief 归还当前资源，当前资源必须不在池中。该方法对 this 线程不安全，
   * 在同一时刻最多只能有一个线程调用该方法。
   *
   * @param t 任何一个在池内的资源，将当前资源插入到它的后面。
   */
  void give_next(Pooled& t)
  {
    assert(t.mPrevPooled.load(std::memory_order_relaxed) == nullptr &&
           t.mNextPooled.load(std::memory_order_relaxed) == nullptr);

    mPrevPooled.store(&t, std::memory_order_release);
    auto next = t.mNextPooled.exchange(this, std::memory_order_release);
    // 到这里 this 就已经可以被认为在池中了
    mNextPooled.store(next, std::memory_order_release);
    if (next != nullptr)
      next->mPrevPooled.store(this, std::memory_order_release);
  }

  /**
   * @brief 丢弃当前资源，重复取出时无操作，在同一时刻最多只能有一个线程调用
   * 该方法。
   */
  void drop_this()
  {
    while (true) {
      auto prev = mPrevPooled.load(std::memory_order_acquire);
      auto next = mNextPooled.load(std::memory_order_acquire);
      auto expected = this;
      if (!prev->mNextPooled.compare_exchange_weak(expected,
                                                   next,
                                                   std::memory_order_release,
                                                   std::memory_order_relaxed))
        continue;
      // assert(expected == this);
      while (!next->mPrevPooled.compare_exchange_weak(
        expected, prev, std::memory_order_relaxed, std::memory_order_relaxed))
        ;
    }
  }

private:
  std::atomic<T*> mPrevPooled;
  std::atomic<T*> mNextPooled;

  Pooled(Pooled* prev, Pooled* next)
    : mPrevPooled(prev)
    , mNextPooled(next)
  {
  }
};

struct Pooled::Pool : Pooled
{
  Pool()
    : Pooled(this, this)
  {
  }
};

} // namespace My
