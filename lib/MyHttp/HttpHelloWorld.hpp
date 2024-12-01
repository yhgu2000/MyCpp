#pragma once

#include "HttpHandler.hpp"

namespace MyHttp {

/**
 * @brief Hello World HTTP 处理器，无论什么请求都只会响应包含 "Hello, World!" 的
 * 200 OK 报文。
 */
class HttpHelloWorld : public HttpHandler
{
public:
  HttpHelloWorld(Socket&& sock, Config& config)
    : HttpHandler(std::move(sock), config, "HttpHelloWorld")
  {
  }

private:
  void do_handle() noexcept override;
};

} // namespace MyHttp
