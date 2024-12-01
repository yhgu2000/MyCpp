#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <thread>

namespace My {

/**
 * @brief 符合标准库具名要求的可定时自旋锁。
 *
 * https://zh.cppreference.com/w/cpp/named_req/BasicLockable
 */
class SpinMutex
{
public:
  class Recursive;
  class Shared;

public:
  void lock() noexcept
  {
    while (mLocked.test_and_set(std::memory_order_acquire))
      ;
  }

  void unlock() noexcept { mLocked.clear(std::memory_order_release); }

  bool try_lock() noexcept
  {
    return !mLocked.test_and_set(std::memory_order_acquire);
  }

  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout) noexcept;

  template<class Clock, class Duration>
  bool try_lock_until(
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept;

protected:
  std::atomic_flag mLocked{ ATOMIC_FLAG_INIT };
};

template<class Rep, class Period>
bool
SpinMutex::try_lock_for(
  const std::chrono::duration<Rep, Period>& timeout) noexcept
{
  return try_lock_until(std::chrono::high_resolution_clock::now() + timeout);
}

template<class Clock, class Duration>
bool
SpinMutex::try_lock_until(
  const std::chrono::time_point<Clock, Duration>& timeout) noexcept
{
  while (mLocked.test_and_set(std::memory_order_acquire))
    if (Clock::now() >= timeout)
      return false;
  return true;
}

/**
 * @brief 符合标准库具名要求的递归自旋锁。
 */
class SpinMutex::Recursive
{
public:
  void lock() noexcept;

  void unlock() noexcept
  {
    assert(mOwner.load(std::memory_order_relaxed) ==
             std::this_thread::get_id() &&
           mCount > 0);
    if (--mCount == 0)
      mOwner.store({}, std::memory_order_release);
  }

  bool try_lock() noexcept;

  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout) noexcept;

  template<class Clock, class Duration>
  bool try_lock_until(
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept;

protected:
  std::atomic<std::thread::id> mOwner;
  unsigned mCount{ 0 };
};

template<class Rep, class Period>
bool
SpinMutex::Recursive::try_lock_for(
  const std::chrono::duration<Rep, Period>& timeout) noexcept
{
  return try_lock_until(std::chrono::high_resolution_clock::now() + timeout);
}

template<class Clock, class Duration>
bool
SpinMutex::Recursive::try_lock_until(
  const std::chrono::time_point<Clock, Duration>& timeout) noexcept
{
  std::thread::id desired = std::this_thread::get_id();
  if (mOwner.load(std::memory_order_relaxed) == desired) {
    ++mCount;
    return true;
  }
  std::thread::id expected;
  do {
    if (Clock::now() >= timeout)
      return false;
    expected = {};
  } while (!mOwner.compare_exchange_weak(
    expected, desired, std::memory_order_acquire, std::memory_order_relaxed));
  mCount = 1;
  return true;
}

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

  void unlock() noexcept
  {
    assert(mCount.load(std::memory_order_relaxed) == UINT_MAX);
    mCount.store(0, std::memory_order_release);
  }

  bool try_lock() noexcept
  {
    unsigned expected = 0, desired = UINT_MAX;
    return mCount.compare_exchange_strong(
      expected, desired, std::memory_order_acquire, std::memory_order_relaxed);
  }

  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout) noexcept;

  template<class Clock, class Duration>
  bool try_lock_until(
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept;

  void lock_shared() noexcept;

  void unlock_shared() noexcept
  {
    assert(mCount.load(std::memory_order_relaxed) != UINT_MAX &&
           mCount.load(std::memory_order_relaxed) != 0);
    mCount.fetch_sub(1, std::memory_order_release);
  }

  bool try_lock_shared() noexcept;

  template<class Rep, class Period>
  bool try_lock_shared_for(
    const std::chrono::duration<Rep, Period>& timeout) noexcept;

  template<class Clock, class Duration>
  bool try_lock_shared_until(
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept;

protected:
  std::atomic<unsigned> mCount{ 0 };
};

template<class Rep, class Period>
bool
SpinMutex::Shared::try_lock_for(
  const std::chrono::duration<Rep, Period>& timeout) noexcept
{
  return try_lock_until(std::chrono::high_resolution_clock::now() + timeout);
}

template<class Clock, class Duration>
bool
SpinMutex::Shared::try_lock_until(
  const std::chrono::time_point<Clock, Duration>& timeout) noexcept
{
  unsigned expected, desired = UINT_MAX;
  do {
    if (Clock::now() >= timeout)
      return false;
    expected = 0;
  } while (!mCount.compare_exchange_weak(
    expected, desired, std::memory_order_acquire, std::memory_order_relaxed));
  return true;
}

template<class Rep, class Period>
bool
SpinMutex::Shared::try_lock_shared_for(
  const std::chrono::duration<Rep, Period>& timeout) noexcept
{
  return try_lock_shared_until(std::chrono::high_resolution_clock::now() +
                               timeout);
}

template<class Clock, class Duration>
bool
SpinMutex::Shared::try_lock_shared_until(
  const std::chrono::time_point<Clock, Duration>& timeout) noexcept
{
  unsigned expected = 0;
  do {
    if (Clock::now() >= timeout)
      return false;
    if (expected == UINT_MAX)
      expected = 0;
  } while (!mCount.compare_exchange_weak(expected,
                                         expected + 1,
                                         std::memory_order_acquire,
                                         std::memory_order_relaxed));
  return true;
}

} // namespace My
