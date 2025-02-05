#pragma once

#include "HttpHandler.hpp"
#include "Server.hpp"

namespace MyHttp {

/**
 * @brief Hello World HTTP 处理器，无论什么请求都只会响应包含 "Hello, World!" 的
 * 200 OK 报文。
 */
class HttpHelloWorld : public HttpHandler
{
public:
  class Server;

  HttpHelloWorld(Socket&& sock,
                 Config& config,
                 std::string logName = "MyHttp::HttpHelloWorld")
    : HttpHandler(std::move(sock), config, std::move(logName))
  {
  }

private:
  void do_handle() noexcept override;
};

class HttpHelloWorld::Server : public MyHttp::Server
{
  void come(Socket&& sock) override;

public:
  HttpHandler::Config mConfig;

  using MyHttp::Server::Server;
};

} // namespace MyHttp
