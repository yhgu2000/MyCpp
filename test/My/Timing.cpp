#include "testutil.hpp"

#include <My/util.hpp>

using namespace My;
using namespace My::util;

/**
 * @brief 测试计时器的性能
 */
BOOST_AUTO_TEST_CASE(performance)
{
  Timing tim;

  constexpr auto kLoops = 1000000;
  auto cost = niming(kLoops, tim(""));
  std::cout << kLoops << " times cost " << cost << ", " << cost / kLoops
            << " per time." << std::endl;
}
