#pragma once

#include <chrono>
#include <iomanip>
#include <ostream>
#include <string>
#include <vector>

namespace My::util {

/**
 * @brief 空操作，用于占位和阻止编译器优化。
 */
void
noop();

using namespace std::string_literals;
using namespace std::chrono_literals;

using std::to_string;

/**
 * @brief 将时间间隔表达为易读的字符串
 */
template<typename R, typename P>
std::string
to_string(const std::chrono::duration<R, P>& dura)
{
  double count =
    std::chrono::duration_cast<std::chrono::nanoseconds>(dura).count();
  if (count < 10000)
    return to_string(count) + "ns";
  else if ((count /= 1000) < 10000)
    return to_string(count) + "us";
  else if ((count /= 1000) < 10000)
    return to_string(count) + "ms";
  else
    return to_string(count / 1000) + "s";
}

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

using Bytes = std::vector<std::uint8_t>;

inline std::string
to_string(const Bytes& bytes)
{
  return std::string(bytes.begin(), bytes.end());
}

inline Bytes
to_bytes(const std::string& str)
{
  return Bytes(str.begin(), str.end());
}

inline Bytes
to_bytes(const void* data, std::size_t size)
{
  return Bytes(reinterpret_cast<const std::uint8_t*>(data),
               reinterpret_cast<const std::uint8_t*>(data) + size);
}

inline Bytes
operator""_b(const char* s, std::size_t n)
{
  return Bytes(s, s + n);
}

} // namespace My::util
