#pragma once

#include "util.hpp"

#include <My/Standing.hpp>
#include <My/log.hpp>
#include <atomic>

namespace MyHttp {

using namespace util;

/**
 * @brief 客户端类，实现了 TCP 连接复用。
 */
class Client : public My::Standing
{
public:
  ba::io_context& mIoCtx;
  const std::string mHost;
  const std::string mPort;

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
  BoostResult<Response> http(Request& req) noexcept;

  /**
   * @brief 异步发送 HTTP 请求。
   *
   * 这个方法是并发安全的，可以在同时在多个线程中被调用。
   *
   * @param req 请求对象。
   * @param cb 回调函数，响应结果作为参数。
   */
  void async_http(Request& req,
                  std::function<void(BoostResult<Response>&&)> cb);

protected:
  My::log::Logger mLogger;

private:
  struct Connection;
  std::atomic<Connection*> mConn{ nullptr };
};

} // namespace MyHttp
