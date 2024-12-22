#pragma once

#include <My/log.hpp>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system/result.hpp>
#include <random>
#include <vector>

namespace MyHttp::util {

namespace ba = boost::asio;
namespace bb = boost::beast;
namespace http = bb::http;

using BoostEC = boost::system::error_code;
template<typename T>
using BoostResult = boost::system::result<T>;

using Executor = ba::any_io_executor;

using Socket = ba::ip::tcp::socket;
using Endpoint = ba::ip::tcp::endpoint;

using BytesBody = http::vector_body<std::uint8_t>;
using Request = http::request<BytesBody>;
using Response = http::response<BytesBody>;

/**
 * @brief 将套接字表达为字符串，用于日志输出。
 */
std::string
strsock(const Socket& sock);

/// 线程本地的快速随机数引擎。
extern thread_local std::minstd_rand gRandFast;
/// 线程本地的安全随机数引擎。
extern thread_local std::mt19937_64 gRandSafe;

/**
 * @brief 异步返回值，用于异步函数。
 *
 * @tparam T 返回值类型。
 */
template<typename T>
struct AsyncReturn : public std::variant<T, std::exception_ptr>
{
  using std::variant<T, std::exception_ptr>::variant;

  /// 检查是否成功。
  explicit operator bool() const noexcept { return this->index() == 0; }

  /// 获取返回值。
  T& operator*() { return std::get<0>(*this); }

  /// 获取返回值。
  T* operator->() { return &std::get<0>(*this); }

  /// 获取返回值。
  T& value() { return std::get<0>(*this); }

  /// 获取异常指针。
  std::exception_ptr& exception() { return std::get<1>(*this); }
};

/**
 * @brief 多线程执行器，创建指定数量的工作线程并运行 io_context::run。
 */
class ThreadsExecutor
{
public:
  ba::io_context mIoCtx;

  ThreadsExecutor(int threads, std::string logName = "MyHttp::ThreadsExecutor")
    : mLogger(std::move(logName), this)
    , mIoCtx(threads)
    , mThreads(threads)
  {
    assert(threads > 0);
  }

  /// 如果析构时正在运行，则尝试停止并阻塞。
  ~ThreadsExecutor() noexcept { stop(); }

  operator Executor() { return mIoCtx.get_executor(); }

  /**
   * @brief 启动执行器，创建工作线程并让它们运行 io_context::run
   * 当前线程立刻返回，不运行工作。
   *
   * @return 启动成功返回 true，若已启动则返回 false。
   */
  bool start();

  /**
   * @brief 停止执行器，调用 io_context::stop 并等待所有线程退出。
   *
   * @return 停止成功返回 true，若已停止则返回 false。
   */
  bool stop();

  /**
   * @brief 阻塞等待所有工作完成，然后停止执行器。
   *
   * @return 停止成功返回 true，若已停止则返回 false。
   */
  bool wait();

private:
  My::log::LoggerMt mLogger;
  std::vector<std::thread> mThreads;
};

} // namespace MyHttp::util
