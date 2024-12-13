#pragma once

#include "util.hpp"

#include <My/Pooled.hpp>
#include <My/log.hpp>
#include <atomic>

namespace MyHttp {

using namespace util;

/**
 * @brief 客户端类，实现了 TCP 连接复用。
 */
class Client
{
public:
  struct Connection : My::Pooled<Connection>
  {
    Connection(ba::io_context& ioCtx)
      : mSocket(ba::make_strand(ioCtx))
      , mTimer(mSocket.get_executor())
    {
    }

    My::log::Logger mLogger{ "Client::Connection", this };
    Socket mSocket;
    ba::steady_timer mTimer;
  };

  ba::io_context& mIoCtx;

  // 目标服务器地址
  const std::string mHost;
  // 目标服务器端口
  const std::string mPort;
  // 连接超时时间（单位：毫秒）
  const ba::steady_timer::duration mTimeout{ 3000 };

  Client(ba::io_context& ioCtx,
         std::string host,
         std::string port,
         const char* loggerName = "Client")
    : mIoCtx(ioCtx)
    , mHost(std::move(host))
    , mPort(std::move(port))
    , mLogger(loggerName, this)
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
  BoostResult<Response> http(const Request& req) noexcept;

  /**
   * @brief 异步发送 HTTP 请求。
   *
   * 这个方法是并发安全的，可以在同时在多个线程中被调用。
   *
   * @param req 请求对象。
   * @param cb 回调续程，响应结果作为参数。
   */
  void async_http(Request req, std::function<void(BoostResult<Response>&&)> cb);

private:
  My::log::LoggerMt mLogger;
  Connection::Pool mPool;
  ba::ip::tcp::resolver mResolver{ mIoCtx };
};

} // namespace MyHttp
