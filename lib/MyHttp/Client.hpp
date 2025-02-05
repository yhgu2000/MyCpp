#pragma once

#ifndef MYHTTP_CLIENT_PRIVATE
#define MYHTTP_CLIENT_PRIVATE private:
#endif

#include "util.hpp"

#include <My/Pooled.hpp>
#include <My/log.hpp>
#include <atomic>
#include <boost/json.hpp>

namespace MyHttp {

using namespace util;
namespace bj = boost::json;

/**
 * @brief 客户端类，实现了 TCP 连接复用。
 */
class Client
{
public:
  struct Config
  {
    /// 目标服务器地址
    std::string mHost;
    /// 目标服务器端口
    std::string mPort;
    /// 每个会话的缓冲区大小
    std::size_t mBufferLimit{ 8 << 10 };
    /// 连接超时限制，超时未连接成功则重试
    ba::steady_timer::duration mTimeout = std::chrono::seconds(3);
    /// 请求重试次数，超出则视为失败
    std::uint32_t mMaxRetry{ 1 };
    /// 保活超时限制，超时无活动的连接会被关闭
    ba::steady_timer::duration mKeepAliveTimeout = std::chrono::seconds(3);

    /// 转换到 JSON 值对象
    bj::value to_jval() const noexcept;
    /// 从 JSON 值对象设置，出错时返回非空异常指针
    void jval_to(const bj::value& jval) noexcept(false);
  };

  // 异步执行器
  Executor mEx;
  // 客户端配置
  const Config& mConfig;
  // 日志器名称
  const std::string mLogName;

  Client(Config& config, Executor ex, std::string logName = "MyHttp::Client")
    : mEx(std::move(ex))
    , mConfig(config)
    , mLogName(std::move(logName))
  {
  }

  /**
   * @brief 阻塞发送 HTTP 请求，返回响应。
   *
   * 这个方法是并发安全的，可以在同时在多个线程中被调用。
   *
   * @param req 请求对象。
   * @return 响应对象。
   */
  BoostResult<Response> http(Request& req) noexcept;

  /**
   * @brief 异步发送 HTTP 请求。
   *
   * 这个方法是并发安全的，可以在同时在多个线程中被调用。
   *
   * @param req 请求对象。
   * @param cb 回调续程，响应结果作为参数。
   */
  void async_http(Request req, std::function<void(BoostResult<Response>&&)> cb);

  /**
   * @brief 清空连接池。
   */
  void clear_connections() { mConnPool.clear(); }

private:
  MYHTTP_CLIENT_PRIVATE

  struct Connection : My::Pooled<Connection>
  {
    Connection(Executor& ex)
      : mTimer(ba::make_strand(ex))
      , mResolver(mTimer.get_executor())
      , mSocket(mTimer.get_executor())
    {
    }

    ba::steady_timer mTimer;
    ba::ip::tcp::resolver mResolver;
    Socket mSocket;
  };

  Connection::Pool mConnPool;
};

} // namespace MyHttp
