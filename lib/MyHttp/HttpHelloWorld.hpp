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

  HttpHelloWorld(Socket&& sock, Config& config)
    : HttpHandler(std::move(sock), config, "MyHttp::HttpHelloWorld")
  {
  }

private:
  void do_handle() noexcept override;
};

class HttpHelloWorld::Server : public MyHttp::Server
{
  HttpHandler::Config& mConfig;

  void come(Socket&& sock) override;

public:
  Server(HttpHandler::Config& config,
         Executor ex,
         std::string logName = "MyHttp::HttpHelloWorld::Server")
    : MyHttp::Server(std::move(ex), std::move(logName))
    , mConfig(config)
  {
  }
};

} // namespace MyHttp
