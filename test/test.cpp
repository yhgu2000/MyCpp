#include "testutil.hpp"

//==============================================================================
// 全局测试夹具
//==============================================================================

struct GlobalFixture
{
  GlobalFixture() {}

  void setup() {}

  void teardown() {}

  ~GlobalFixture() {}
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);

BOOST_AUTO_TEST_CASE(example)
{
  // BOOST_TEST(false);// 在失败后会继续执行
  BOOST_REQUIRE(true); // 在失败后执行会中止
}

//==============================================================================
// 功能性测试，测试目标功能是否被正确实现。
//==============================================================================

struct BasicFixture
{
  BasicFixture() {}

  void setup() {}

  void teardown() {}

  ~BasicFixture() {}
};

BOOST_FIXTURE_TEST_SUITE(basic, BasicFixture)

BOOST_AUTO_TEST_CASE(case0) {}

BOOST_AUTO_TEST_SUITE_END()

//==============================================================================
// 稳定性测试，针对大规模和复杂输入的测试。
//==============================================================================

BOOST_AUTO_TEST_SUITE(stable)

BOOST_AUTO_TEST_CASE(case0) {}

BOOST_AUTO_TEST_SUITE_END()

//==============================================================================
// 健壮性测试，针对错误和非常规输入的测试。
//==============================================================================

BOOST_AUTO_TEST_SUITE(robust)

BOOST_AUTO_TEST_CASE(case0) {}

BOOST_AUTO_TEST_SUITE_END()
