#pragma once

#include <boost/log/attributes/constant.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

namespace My::log {

enum Level
{
  verb = 0,  // 非常详细，有大量输出的，会影响性能。
  info = 1,  // 例行性事件，中等规模输出，可能会影响性能。
  noti = 2,  // 值得注意的事件，输出不频繁，不会影响性能。
  warn = 3,  // 轻微错误，不会影响系统运行。
  crit = 4,  // 关键错误，可能会导致系统局部失灵。
  fatal = 5, // 致命错误，可能会导致系统崩溃。
  debug = 6, // 临时调试，仅在开发时使用，在发布时删除。
};

class Logger : public boost::log::sources::severity_channel_logger<Level>
{
  using _T = Logger;
  using _S = boost::log::sources::severity_channel_logger<Level>;

public:
  Logger() = default;

  Logger(std::string channel)
    : _S(boost::log::keywords::channel = std::move(channel))
  {
  }

  /**
   * @param channel 日志通道，和代码静态关联。
   * @param object 对象标识符，运行时动态生成。
   */
  Logger(std::string channel, const void* object)
    : _S(boost::log::keywords::channel = std::move(channel))
  {
    add_attribute("ObjectID",
                  boost::log::attributes::constant<const void*>(object));
  }
};

class LoggerMt : public boost::log::sources::severity_channel_logger_mt<Level>
{
  using _T = LoggerMt;
  using _S = boost::log::sources::severity_channel_logger_mt<Level>;

public:
  LoggerMt() = default;

  LoggerMt(std::string channel)
    : _S(boost::log::keywords::channel = std::move(channel))
  {
  }

  /**
   * @param channel 日志通道，和代码静态关联。
   * @param object 对象标识符，运行时动态生成。
   */
  LoggerMt(std::string channel, const void* object)
    : _S(boost::log::keywords::channel = std::move(channel))
  {
    add_attribute("ObjectID",
                  boost::log::attributes::constant<const void*>(object));
  }
};

/**
 * @brief 日志格式化函数，使用示例：
 *
 * ```
 * boost::log::add_console_log(std::clog,
 *   boost::log::keywords::format = format);
 * ```
 */
void
format(const boost::log::record_view&, boost::log::formatting_ostream&);

} // namespace My::log
