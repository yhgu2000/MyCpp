#include "SpinMutex.hpp"

namespace My {

void
SpinMutex::Recursive::lock() noexcept
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

bool
SpinMutex::Recursive::try_lock() noexcept
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

void
SpinMutex::Shared::lock_shared() noexcept
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

bool
SpinMutex::Shared::try_lock_shared() noexcept
{
  unsigned expected = 0;
  if (expected == UINT_MAX)
    return false;
  return mCount.compare_exchange_strong(expected,
                                        expected + 1,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed);
}

} // namespace My
