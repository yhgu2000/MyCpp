#pragma once

#include "util.hpp"

#include <My/log.hpp>

#include <thread>
#include <vector>

namespace MyHttp {

using namespace util;

/**
 * @brief 服务器类，监听端口并创建处理器。
 */
class Server
{
public:
  Server(Executor ex, std::string logName = "MyHttp::Server")
    : mLogger(std::move(logName), this)
    , mAcpt(ba::make_strand(std::move(ex)))
  {
  }

  /**
   * @brief 启动服务，监听端口。
   *
   * @param endpoint 监听端点。
   * @param backlog 最大等待连接数。
   * @return 启动成功返回假值，否则返回错误码真值。
   */
  BoostEC start(const Endpoint& endpoint,
                int backlog = ba::socket_base::max_listen_connections);

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
