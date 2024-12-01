#pragma once

#include "HttpHandler.hpp"

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
  HttpMatpowsum(Socket&& sock, Config& config)
    : HttpHandler(std::move(sock), config, "HttpMatpowsum")
  {
  }

private:
  void do_handle() noexcept override;
};

} // namespace MyHttp
