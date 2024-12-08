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

  template<typename T,
           unsigned B = 1,
           typename = std::enable_if_t<std::is_scalar_v<T>>>
  struct Bit;

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
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout) noexcept
  {
    return try_lock_until(std::chrono::high_resolution_clock::now() + timeout);
  }

  template<class Clock, class Duration>
  bool try_lock_until(
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept;

protected:
  std::atomic_flag mLocked{ ATOMIC_FLAG_INIT };
};

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
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout) noexcept
  {
    return try_lock_until(std::chrono::high_resolution_clock::now() + timeout);
  }

  template<class Clock, class Duration>
  bool try_lock_until(
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept;

protected:
  std::atomic<std::thread::id> mOwner;
  unsigned mCount{ 0 };
};

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
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout) noexcept
  {
    return try_lock_until(std::chrono::high_resolution_clock::now() + timeout);
  }

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
    const std::chrono::duration<Rep, Period>& timeout) noexcept
  {
    return try_lock_shared_until(std::chrono::high_resolution_clock::now() +
                                 timeout);
  }

  template<class Clock, class Duration>
  bool try_lock_shared_until(
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept;

protected:
  std::atomic<unsigned> mCount{ 0 };
};

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

/**
 * @brief 使用标量类型的一个位作为自旋锁。
 *
 * @tparam T 标量类型，可以是整数或指针。
 * @tparam B 位号，0 为最低位。
 */
template<typename T, unsigned B, typename>
struct SpinMutex::Bit
{
  static bool test(T t)
  {
    if constexpr (std::is_pointer_v<T>)
      return reinterpret_cast<std::uintptr_t>(t) & (1 << B);
    else
      return t & (T(1) << B);
  }

  static T set(T t)
  {
    if constexpr (std::is_pointer_v<T>)
      return reinterpret_cast<T>(reinterpret_cast<std::uintptr_t>(t) |
                                 (std::uintptr_t(1) << B));
    else
      return t | (T(1) << B);
  }

  static T unset(T t)
  {
    if constexpr (std::is_pointer_v<T>)
      return reinterpret_cast<T>(reinterpret_cast<std::uintptr_t>(t) &
                                 ~(std::uintptr_t(1) << B));
    else
      return t & ~(T(1) << B);
  }

  /**
   * @brief 获取原始值。
   */
  static T masked(const std::atomic<T>& t) noexcept
  {
    return unset(t.load(std::memory_order_relaxed));
  }

  /**
   * @brief 设置原始值。
   */
  static void masked(std::atomic<T>& t, T v) noexcept
  {
    T expected{};
    do
      if constexpr (std::is_pointer_v<T>)
        v = reinterpret_cast<T>(reinterpret_cast<std::uintptr_t>(v) |
                                reinterpret_cast<std::uintptr_t>(expected) &
                                  (std::uintptr_t(1) << B));
      else
        v = v | (expected & (T(1) << B));
    while (!t.compare_exchange_weak(
      expected, v, std::memory_order_relaxed, std::memory_order_relaxed));
  }

  /**
   * @brief 检查是否被锁定。
   */
  static bool locked(const std::atomic<T>& t) noexcept
  {
    return test(t.load(std::memory_order_relaxed));
  }

  static void lock(std::atomic<T>& t) noexcept
  {
    T expected{};
    do
      expected = unset(expected);
    while (!t.compare_exchange_weak(expected,
                                    set(expected),
                                    std::memory_order_acquire,
                                    std::memory_order_relaxed));
  }

  static void unlock(std::atomic<T>& t) noexcept
  {
    if constexpr (std::is_pointer_v<T>) {
      auto v = t.load(std::memory_order_relaxed);
      assert(test(v));
      t.store(unset(v), std::memory_order_release);
    } else {
      assert(test(t.load(std::memory_order_relaxed)));
      t.fetch_and(~(T(1) << B), std::memory_order_release);
    }
  }

  static bool try_lock(std::atomic<T>& t) noexcept
  {
    T expected = unset(t.load(std::memory_order_relaxed));
    return t.compare_exchange_strong(expected,
                                     set(expected),
                                     std::memory_order_acquire,
                                     std::memory_order_relaxed);
  }

  template<class Rep, class Period>
  static bool try_lock_for(
    std::atomic<T>& t,
    const std::chrono::duration<Rep, Period>& timeout) noexcept
  {
    return try_lock_until(t,
                          std::chrono::high_resolution_clock::now() + timeout);
  }

  template<class Clock, class Duration>
  static bool try_lock_until(
    std::atomic<T>& t,
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept;

  std::atomic<T>& mT;

  Bit(std::atomic<T>& t) noexcept
    : mT(t)
  {
  }

  bool locked() const noexcept { return locked(mT); }

  T masked() const noexcept { return masked(mT); }

  void masked(T v) noexcept { masked(mT, v); }

  void lock() noexcept { lock(mT); }

  void unlock() noexcept { unlock(mT); }

  bool try_lock() noexcept { return try_lock(mT); }

  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout) noexcept
  {
    return try_lock_for(mT, timeout);
  }

  template<class Clock, class Duration>
  bool try_lock_until(
    const std::chrono::time_point<Clock, Duration>& timeout) noexcept
  {
    return try_lock_until(mT, timeout);
  }
};

template<typename T, unsigned B, typename U>
template<class Clock, class Duration>
bool
SpinMutex::Bit<T, B, U>::try_lock_until(
  std::atomic<T>& t,
  const std::chrono::time_point<Clock, Duration>& timeout) noexcept
{
  T expected{};
  do {
    expected = unset(expected);
    if (Clock::now() >= timeout)
      return false;
  } while (!t.compare_exchange_weak(expected,
                                    set(expected),
                                    std::memory_order_acquire,
                                    std::memory_order_relaxed));
  return true;
}

} // namespace My
