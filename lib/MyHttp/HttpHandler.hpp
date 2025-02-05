#pragma once

#include "util.hpp"

#include <My/log.hpp>
#include <boost/json.hpp>

namespace MyHttp {

using namespace util;
namespace bj = boost::json;

/**
 * @brief HTTP/1.1 处理器，实现了 Keep-Alive 机制。
 */
class HttpHandler : public std::enable_shared_from_this<HttpHandler>
{
public:
  struct Config
  {
    /// 每个会话的缓冲区大小
    std::size_t mBufferLimit{ 8 << 10 };
    /// 保活超时限制，超时无活动的连接会被关闭
    std::uint32_t mKeepAliveTimeout{ 3 };
    /// 保活次数限制，超过次数的连接会被关闭，UINT32_MAX 表示无限制
    std::uint32_t mKeepAliveMax{ UINT32_MAX };

    /// 转换到 JSON 值对象
    bj::value to_jval() const noexcept;
    /// 从 JSON 值对象设置，出错时抛出异常
    void jval_to(const bj::value& jval) noexcept(false);
  };

  /**
   * @brief 启动异步处理，应当在类外被调用（如监听器）。
   */
  void start();

  /**
   * @brief 强行停止处理，关闭底层套接字，应当在类外被调用。
   */
  void stop();

protected:
  My::log::Logger mLogger;
  Request mRequest;
  Response mResponse;

  /**
   * @param sock 已经建立连接的套接字。
   * @param config 配置项，运行中不可更改。
   * @param logName 日志名称。
   */
  HttpHandler(Socket&& sock,
              Config& config,
              std::string logName = "MyHttp::HttpHandler")
    : mLogger(std::move(logName), this)
    , mConfig(config)
    , mStream(std::move(sock))
    , mBuffer(config.mBufferLimit)
  {
  }

  virtual ~HttpHandler() = default;

  /**
   * @brief 在子类中实现请求的处理回调。
   *
   * 当前的请求为 `mRequest`，响应数据应当设置到 `mResponse`。在处理结束
   * 后（无论成功还是失败），`on_handle()` 都应该被调用仅被调用一次。如果需要
   * 错误中止处理，则在调用 `on_handle()` 时传递异常指针。响应报文的
   * `Content-Length` 头字段会被 `on_handle()` 自动设置，子类无需处理。
   *
   * @warning 该方法必须是线程安全的且在不同对象上可重入。
   *
   * @note `do_handle` 其实是一种 CPS，而 `on_handle` 就是它的续程。
   * 之所以这么拐弯抹角地设计，而不是在 `do_handle` 外直接 try-catch，
   * 是因为 `do_handle` 可能是一个异步过程，例如在处理过程中异步地读写一个文件。
   */
  virtual void do_handle() noexcept = 0;

  /**
   * @brief 子类应当在处理完成（无论成功还是失败）后调用，且保证在任何情况下
   * 仅调用一次。如果请求处理成功，那么 `mResponse` 应被置为为响应数据。
   *
   * @param eptr 异常指针，不为空时视为处理出错。
   */
  void on_handle(std::exception_ptr eptr) noexcept;

private:
  const Config& mConfig;
  bb::tcp_stream mStream;
  bb::flat_buffer mBuffer;
  std::uint16_t mKeepAliveCount{ 0 };

  std::chrono::high_resolution_clock::time_point mTimingBegin;
  std::chrono::high_resolution_clock::time_point mTimingHandleBegin;

  void do_read();
  void on_read(const BoostEC& ec, std::size_t len);
  void do_write();
  void on_write(const BoostEC& ec, std::size_t len);
  void do_close(const char* reason);
};

} // namespace ORFA
