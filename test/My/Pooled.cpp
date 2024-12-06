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
  pool.give(std::make_shared<EmptyRC>(1));
  auto rc = pool.take();
  BOOST_REQUIRE(rc->mI == 1);
}

BOOST_AUTO_TEST_CASE(performance) {}
