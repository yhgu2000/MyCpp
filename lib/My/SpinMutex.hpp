#pragma once

#include <atomic>
#include <cassert>
#include <thread>

namespace My {

/**
 * @brief 符合标准库具名要求的自旋锁。
 *
 * https://zh.cppreference.com/w/cpp/named_req/BasicLockable
 */
class SpinMutex
{
public:
  class Recursive;
  class Shared;

  // TODO 以下三个类的实现
  class Timed;
  class TimedRecursive;
  class TimedShared;

public:
  void lock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    while (mLocked.test_and_set(order))
      ;
  }

  bool try_lock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    return !mLocked.test_and_set(order);
  }

  void unlock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    mLocked.clear(order);
  }

private:
  std::atomic_flag mLocked{ ATOMIC_FLAG_INIT };
};

/**
 * @brief 符合标准库具名要求的递归自旋锁。
 */
class SpinMutex::Recursive
{
public:
  void lock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    std::thread::id desired = std::this_thread::get_id();
    if (mOwner.load(order) == desired) {
      ++mCount;
      return;
    }
    std::thread::id expected;
    do
      expected = {};
    while (!mOwner.compare_exchange_weak(expected, desired, order));
    mCount = 1;
  }

  bool try_lock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    std::thread::id desired = std::this_thread::get_id();
    if (mOwner.load(order) == desired) {
      ++mCount;
      return true;
    }
    std::thread::id expected;
    if (!mOwner.compare_exchange_weak(expected, desired, order))
      return false;
    mCount = 1;
    return true;
  }

  void unlock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    assert(mOwner.load(std::memory_order_relaxed) ==
             std::this_thread::get_id() &&
           mCount > 0);
    if (--mCount == 0)
      mOwner.store({}, order);
  }

private:
  std::atomic<std::thread::id> mOwner;
  unsigned mCount{ 0 };
};

/**
 * @brief 符合标准库具名要求的共享自旋锁。
 */
class SpinMutex::Shared
{
public:
  void lock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    unsigned expected, desired = UINT_MAX;
    do
      expected = 0;
    while (!mCount.compare_exchange_weak(expected, desired, order));
  }

  bool try_lock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    unsigned expected = 0, desired = UINT_MAX;
    return mCount.compare_exchange_weak(expected, desired, order);
  }

  void unlock(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    assert(mCount.load(std::memory_order_relaxed) == UINT_MAX);
    mCount.store(0, order);
  }

  void lock_shared(std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    unsigned expected = mCount.load(order);
    do {
      if (expected == UINT_MAX)
        expected = 0;
    } while (!mCount.compare_exchange_weak(expected, expected + 1, order));
  }

  bool try_lock_shared(
    std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    unsigned expected = mCount.load(order);
    if (expected == UINT_MAX)
      return false;
    return mCount.compare_exchange_weak(expected, expected + 1, order);
  }

  void unlock_shared(
    std::memory_order order = std::memory_order_seq_cst) noexcept
  {
    assert(mCount.load(std::memory_order_relaxed) != UINT_MAX &&
           mCount.load(std::memory_order_relaxed) != 0);
    mCount.fetch_sub(1, order);
  }

private:
  std::atomic<unsigned> mCount;
};

} // namespace My
