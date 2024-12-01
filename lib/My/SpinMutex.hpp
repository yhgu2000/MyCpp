#pragma once

#include <atomic>
#include <cassert>
#include <climits>
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
  class RecursiveTimed;
  class SharedTimed;

public:
  void lock() noexcept
  {
    while (mLocked.test_and_set(std::memory_order_acquire))
      ;
  }

  bool try_lock() noexcept
  {
    return !mLocked.test_and_set(std::memory_order_acquire);
  }

  void unlock() noexcept { mLocked.clear(std::memory_order_release); }

private:
  std::atomic_flag mLocked{ ATOMIC_FLAG_INIT };
};

/**
 * @brief 符合标准库具名要求的递归自旋锁。
 */
class SpinMutex::Recursive
{
public:
  void lock() noexcept
  {
    std::thread::id desired = std::this_thread::get_id();
    if (mOwner.load(std::memory_order_relaxed) == desired) {
      ++mCount;
      return;
    }
    std::thread::id expected;
    do
      expected = {};
    while (!mOwner.compare_exchange_weak(
      expected, desired, std::memory_order_acquire, std::memory_order_relaxed));
    mCount = 1;
  }

  bool try_lock() noexcept
  {
    std::thread::id desired = std::this_thread::get_id();
    if (mOwner.load(std::memory_order_relaxed) == desired) {
      ++mCount;
      return true;
    }
    std::thread::id expected;
    if (!mOwner.compare_exchange_strong(expected,
                                        desired,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed))
      return false;
    mCount = 1;
    return true;
  }

  void unlock() noexcept
  {
    assert(mOwner.load(std::memory_order_relaxed) ==
             std::this_thread::get_id() &&
           mCount > 0);
    if (--mCount == 0)
      mOwner.store({}, std::memory_order_release);
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
  void lock() noexcept
  {
    unsigned expected, desired = UINT_MAX;
    do
      expected = 0;
    while (!mCount.compare_exchange_weak(
      expected, desired, std::memory_order_acquire, std::memory_order_relaxed));
  }

  bool try_lock() noexcept
  {
    unsigned expected = 0, desired = UINT_MAX;
    return mCount.compare_exchange_strong(
      expected, desired, std::memory_order_acquire, std::memory_order_relaxed);
  }

  void unlock() noexcept
  {
    assert(mCount.load(std::memory_order_relaxed) == UINT_MAX);
    mCount.store(0, std::memory_order_release);
  }

  void lock_shared() noexcept
  {
    unsigned expected = 0;
    do
      if (expected == UINT_MAX)
        expected = 0;
    while (!mCount.compare_exchange_weak(expected,
                                         expected + 1,
                                         std::memory_order_acquire,
                                         std::memory_order_relaxed));
  }

  bool try_lock_shared() noexcept
  {
    unsigned expected = 0;
    if (expected == UINT_MAX)
      return false;
    return mCount.compare_exchange_strong(expected,
                                          expected + 1,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed);
  }

  void unlock_shared() noexcept
  {
    assert(mCount.load(std::memory_order_relaxed) != UINT_MAX &&
           mCount.load(std::memory_order_relaxed) != 0);
    mCount.fetch_sub(1, std::memory_order_release);
  }

private:
  std::atomic<unsigned> mCount;
};

} // namespace My
