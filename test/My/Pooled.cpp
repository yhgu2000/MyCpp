#include "testutil.hpp"

#include <My/Pooled.hpp>
#include <thread>

using namespace My;

struct EmptyRC : public Pooled<EmptyRC>
{
  EmptyRC(int i)
    : mI(i)
  {
  }

  int mI;
};

BOOST_AUTO_TEST_CASE(basic)
{
  EmptyRC::Pool pool;

  auto rc1 = std::make_shared<EmptyRC>(1);
  pool.give(rc1);
  BOOST_TEST(EmptyRC::Pool::is_in(*rc1));

  BOOST_TEST(pool.take() == rc1);
  BOOST_TEST(!EmptyRC::Pool::is_in(*rc1));
  pool.give(rc1);

  auto rc2 = std::make_shared<EmptyRC>(2);
  pool.give(rc2);
  BOOST_TEST(EmptyRC::Pool::is_in(*rc2));

  EmptyRC::Pool::drop(*rc1);
  BOOST_TEST(!EmptyRC::Pool::is_in(*rc1));

  BOOST_TEST(pool.take() == rc2);
}

BOOST_AUTO_TEST_CASE(concurrent)
{
  EmptyRC::Pool pool;

  std::vector<std::thread> threads(std::thread::hardware_concurrency());
  for (auto& t : threads) {
    t = std::thread([&] {
      for (int i = 0; i < 1000; ++i) {
        auto rc = pool.take();
        if (!rc)
          rc = std::make_shared<EmptyRC>(i);
        pool.give(rc);
      }
    });
  }

  for (auto i = threads.size(); --i != SIZE_MAX;) {
    for (auto& rc : pool)
      BOOST_TEST(rc.mI >= 0);
  }

  for (auto& t : threads)
    t.join();
  BOOST_TEST(pool.count() <= threads.size());
}

BOOST_AUTO_TEST_CASE(performance)
{
  auto loopsEnv = std::getenv("LOOPS");
  auto loops = loopsEnv ? std::atoi(loopsEnv) : 10000;
  auto threadsEnv = std::getenv("THREADS");
  auto threadsNum =
    threadsEnv ? std::atoi(threadsEnv) : std::thread::hardware_concurrency();

  EmptyRC::Pool pool;
  auto ns = timing({
              std::vector<std::thread> threads(threadsNum);
              for (auto& t : threads) {
                t = std::thread([&] {
                  for (int i = 0; i < loops; ++i) {
                    auto rc = pool.take();
                    if (!rc)
                      rc = std::make_shared<EmptyRC>(i);
                    pool.give(rc);
                  }
                });
              }
              for (auto& t : threads)
                t.join();
            }).count();

  std::cout << threadsNum << " threads perform " << loops
            << " loops, with total " << loops * threadsNum / (double(ns) / 1e9)
            << " throughput per second and " << pool.count()
            << " items in pool." << std::endl;
}
