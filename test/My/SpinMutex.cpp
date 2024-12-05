#include "testutil.hpp"

#include <My/SpinMutex.hpp>
#include <mutex>
#include <thread>
#include <vector>

using namespace My;

struct Progression : std::vector<std::uint64_t>
{
  Progression(std::uint64_t x, std::size_t n)
    : std::vector<std::uint64_t>(n)
  {
    BOOST_ASSERT(n > 2);
    for (std::size_t i = 0; i < n; ++i)
      (*this)[i] = i * x;
  }

  void assign(std::uint64_t x)
  {
    for (std::size_t i = 0; i < size(); ++i)
      (*this)[i] = i * x;
  }

  bool check() const
  {
    auto x = (*this)[1];
    for (std::size_t i = 0; i < size(); ++i)
      if ((*this)[i] != i * x)
        return false;
    return true;
  }
};

BOOST_AUTO_TEST_CASE(spin)
{
  std::vector<std::thread> threads(std::thread::hardware_concurrency());

  SpinMutex mutex;
  Progression series(randgen::range(0, 100), threads.size());
  BOOST_ASSERT(series.check());

  for (auto& thread : threads)
    thread = std::thread([&] {
      for (std::size_t i = 0; i < 1000; ++i) {
        switch (i % 4) {
          case 0:
            mutex.lock();
            break;
          case 1:
            if (!mutex.try_lock())
              continue;
            break;
          case 2:
            if (!mutex.try_lock_for(1ms))
              continue;
            break;
          case 3:
            if (!mutex.try_lock_until(std::chrono::steady_clock::now() + 1ms))
              continue;
            break;
        }

        BOOST_TEST(series.check());
        series.assign(randgen::range(0, 100));
        mutex.unlock();
      }
    });
  for (auto& thread : threads)
    thread.join();
}

BOOST_AUTO_TEST_CASE(recursive)
{
  std::vector<std::thread> threads(std::thread::hardware_concurrency());

  SpinMutex::Recursive mutex;
  Progression series(randgen::range(0, 100), threads.size());

  for (auto& thread : threads)
    thread = std::thread([&] {
      for (std::size_t i = 0; i < 1000; ++i) {
        switch (i % 4) {
          case 0:
            mutex.lock();
            break;
          case 1:
            if (!mutex.try_lock())
              continue;
            break;
          case 2:
            if (!mutex.try_lock_for(1ms))
              continue;
            break;
          case 3:
            if (!mutex.try_lock_until(std::chrono::steady_clock::now() + 1ms))
              continue;
            break;
        }

        for (int i = 0; i < 3; ++i) {
          mutex.lock();
          BOOST_TEST(series.check());
          series.assign(randgen::range(0, 100));
        }

        for (int i = 0; i < 3; ++i)
          mutex.unlock();
        mutex.unlock();
      }
    });
  for (auto& thread : threads)
    thread.join();
}

BOOST_AUTO_TEST_CASE(shared)
{
  std::vector<std::thread> threads(std::thread::hardware_concurrency());

  SpinMutex::Shared mutex;
  Progression series(randgen::range(0, 100), threads.size());

  for (auto& thread : threads)
    thread = std::thread([&] {
      for (std::size_t i = 0; i < 1000; ++i) {
        auto j = i / 3;
        if (j & 1) {
          switch (i % 4) {
            case 0:
              mutex.lock();
              break;
            case 1:
              if (!mutex.try_lock())
                continue;
              break;
            case 2:
              if (!mutex.try_lock_for(1ms))
                continue;
              break;
            case 3:
              if (!mutex.try_lock_until(std::chrono::steady_clock::now() + 1ms))
                continue;
              break;
          }

          BOOST_TEST(series.check());
          series.assign(randgen::range(0, 100));
          mutex.unlock();
        }

        else {
          switch (i % 4) {
            case 0:
              mutex.lock_shared();
              break;
            case 1:
              if (!mutex.try_lock_shared())
                continue;
              break;
            case 2:
              if (!mutex.try_lock_shared_for(1ms))
                continue;
              break;
            case 3:
              if (!mutex.try_lock_shared_until(
                    std::chrono::steady_clock::now() + 1ms))
                continue;
              break;
          }

          BOOST_TEST(series.check());
          mutex.unlock_shared();
        }
      }
    });

  for (auto& thread : threads)
    thread.join();
}

BOOST_AUTO_TEST_CASE(spin_bit)
{
  {
    std::vector<std::thread> threads(std::thread::hardware_concurrency());

    SpinMutex::Bit<int, 10> mutex;
    Progression series(randgen::range(0, 100), threads.size());
    BOOST_ASSERT(series.check());

    for (auto& thread : threads)
      thread = std::thread([&] {
        for (std::size_t i = 0; i < 1000; ++i) {
          switch (i % 4) {
            case 0:
              mutex.lock();
              break;
            case 1:
              if (!mutex.try_lock())
                continue;
              break;
            case 2:
              if (!mutex.try_lock_for(1ms))
                continue;
              break;
            case 3:
              if (!mutex.try_lock_until(std::chrono::steady_clock::now() + 1ms))
                continue;
              break;
          }

          BOOST_TEST(series.check());
          series.assign(randgen::range(0, 100));
          mutex.unlock();
        }
      });
    for (auto& thread : threads)
      thread.join();
  }

  {
    std::vector<std::thread> threads(std::thread::hardware_concurrency());

    SpinMutex::Bit<Progression*, 2> mutex;
    Progression series(randgen::range(0, 100), threads.size());
    BOOST_ASSERT(series.check());

    for (auto& thread : threads)
      thread = std::thread([&] {
        for (std::size_t i = 0; i < 1000; ++i) {
          switch (i % 4) {
            case 0:
              mutex.lock();
              break;
            case 1:
              if (!mutex.try_lock())
                continue;
              break;
            case 2:
              if (!mutex.try_lock_for(1ms))
                continue;
              break;
            case 3:
              if (!mutex.try_lock_until(std::chrono::steady_clock::now() + 1ms))
                continue;
              break;
          }

          BOOST_TEST(series.check());
          series.assign(randgen::range(0, 100));
          mutex.unlock();
        }
      });
    for (auto& thread : threads)
      thread.join();
  }
}
