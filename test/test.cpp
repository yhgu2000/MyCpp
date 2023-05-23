#include "util.hpp"

//==============================================================================
// 功能性测试
//==============================================================================

BOOST_AUTO_TEST_SUITE(functionality)

BOOST_AUTO_TEST_CASE(case0)
{
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()

//==============================================================================
// 稳定性测试
//==============================================================================

BOOST_AUTO_TEST_SUITE(stablity)

BOOST_AUTO_TEST_CASE(case0)
{
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()

//==============================================================================
// 健壮性测试
//==============================================================================

BOOST_AUTO_TEST_SUITE(robustness)

BOOST_AUTO_TEST_CASE(case0)
{
  BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()
