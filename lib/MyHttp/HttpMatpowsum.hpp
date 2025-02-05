#pragma once

#include "HttpHandler.hpp"
#include "Server.hpp"

namespace MyHttp {

/**
 * @brief 矩阵幂和算法的 HTTP 处理器，可用于压力负载测试。
 *
 * 该处理器从 URL 查询参数中获取矩阵阶数 `k` 和幂次 `n`，返回
 * $\sum [1/k]_{k,k}^n$ 的计算结果（即 k）。
 */
class HttpMatpowsum : public HttpHandler
{
public:
  class Server;

  HttpMatpowsum(Socket&& sock,
                Config& config,
                std::string logName = "MyHttp::HttpMatpowsum")
    : HttpHandler(std::move(sock), config, std::move(logName))
  {
  }

private:
  void do_handle() noexcept override;
};

class HttpMatpowsum::Server : public MyHttp::Server
{
  void come(Socket&& sock) override;

public:
  HttpHandler::Config mConfig;

  using MyHttp::Server::Server;
};

} // namespace MyHttp
