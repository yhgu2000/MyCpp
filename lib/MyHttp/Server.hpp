#pragma once

#include "util.hpp"

#include <My/Standing.hpp>
#include <My/log.hpp>

#include <thread>
#include <vector>

namespace MyHttp {

using namespace util;

/**
 * @brief 服务器类，监听端口并创建处理器。
 */
class Server : public My::Standing
{
public:
  ba::io_context& mIoCtx;

  Server(ba::io_context& ioCtx, const char* loggerName = "Server")
    : mIoCtx(ioCtx)
    , mLogger(loggerName, this)
    , mAcpt(ioCtx)
  {
  }

  /// 如果析构时服务器正在运行，则尝试停止并阻塞。
  virtual ~Server() noexcept;

  /**
   * @brief 启动服务，监听端口。
   *
   * @param endpoint 监听端点。
   */
  void start(const Endpoint& endpoint);

  /**
   * @brief 停止监听。
   */
  void stop();

protected:
  My::log::Logger mLogger;

  /**
   * @brief 有新连接到来时调用该函数，子类应当根据实际情况创建相应的处理器。
   *
   * @param sock 新连接的套接字。
   */
  virtual void come(Socket&& sock) = 0;

private:
  ba::ip::tcp::acceptor mAcpt;

  void do_accept();
  void on_accept(const BoostEC& ec, Socket&& sock);
};

} // namespace MyHttp
