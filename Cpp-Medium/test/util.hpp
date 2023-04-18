#pragma once

#include <boost/test/unit_test.hpp>

#include "project.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_set>

using namespace std::chrono_literals;
using namespace std::string_literals;

using HRC = std::chrono::high_resolution_clock;

/**
 * @brief 计时宏，宏参数可以是表达式，也可以是语句块
 */
#define timing(code)                                                           \
  [&]() {                                                                      \
    auto __timing_begin__ = HRC::now();                                        \
    code;                                                                      \
    auto __timing_end__ = HRC::now();                                          \
    return __timing_end__ - __timing_begin__;                                  \
  }()

/**
 * @brief n 次计时宏，宏参数可以是表达式，也可以是语句块
 */
#define niming(n, code)                                                        \
  [&]() {                                                                      \
    auto __niming_begin__ = HRC::now();                                        \
    for (std::size_t __niming_n__ = 0; __niming_n__ < n; ++__niming_n__)       \
      code;                                                                    \
    auto __niming_end__ = HRC::now();                                          \
    return __niming_end__ - __niming_begin__;                                  \
  }()

/**
 * @brief 以易读形式输出时间间隔
 */
template<typename R, typename P>
std::ostream&
operator<<(std::ostream& out, const std::chrono::duration<R, P>& dura)
{
  out << std::fixed << std::setprecision(2);
  double count =
    std::chrono::duration_cast<std::chrono::nanoseconds>(dura).count();
  if (count < 10000)
    out << count << "ns";
  else if ((count /= 1000) < 10000)
    out << count << "us";
  else if ((count /= 1000) < 10000)
    out << count << "ms";
  else
    out << count / 1000 << "s";
  return out;
}

/**
 * @brief 随机数生成工具
 */
namespace genrand {

extern thread_local std::default_random_engine gRand;

/**
 * @brief 生成随机的布尔值
 */
inline bool
tf()
{
  return gRand() & 1;
}

/**
 * @brief 生成0~1之间的均匀随机变量。
 */
inline double
norm()
{
  return std::uniform_real_distribution<>(0, 1)(gRand);
}

/**
 * @brief 生成[0,end)间的随机整数。
 */
inline std::size_t
index(std::size_t end)
{
  return std::uniform_int_distribution<std::size_t>(0, end - 1)(gRand);
}

/**
 * @brief 生成期望将0~1均匀分成n+1份的随机分割点数列，返回的数列已按升序排序。
 */
std::vector<double>
split(std::size_t n);

}
