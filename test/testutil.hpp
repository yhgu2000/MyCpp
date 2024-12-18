#pragma once

#include <boost/test/unit_test.hpp>

#include "project.h"
#include <My/Timing.hpp>
#include <My/log.hpp>
#include <My/util.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_set>

using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace My::util;

#define timing(code) MY_TIMING(code)
#define niming(n, code) MY_NIMING(n, code)

/**
 * @brief 用于阻止一个求值被编译器优化掉。
 */
template<typename T>
inline T&&
noopt(T&& a)
{
  extern void noopt_impl(void*);
  noopt_impl(&a);
  return a;
}

/**
 * @brief 设置日志输出阈值。
 */
void
init_loglevel(int loglevel = My::log::Level::verb);

/**
 * @brief 重设日志输出阈值。
 */
void
reset_loglevel(int loglevel = My::log::Level::noti);

/**
 * @brief 随机数生成工具
 */
namespace randgen {

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

inline std::size_t
range(std::size_t begin, std::size_t end)
{
  return std::uniform_int_distribution<std::size_t>(begin, end)(gRand);
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

} // namespace randgen
