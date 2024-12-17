#define MYHTTP_CLIENT_PRIVATE public:
#include "Client.hpp"

#include <My/err.hpp>
#include <My/util.hpp>

using namespace My::util;
using namespace My::log;

namespace MyHttp {

namespace {

using Conn = std::shared_ptr<Client::Connection>;

struct AsyncHttp
{};

template<>
struct _Client<AsyncHttp> : std::enable_shared_from_this<_Client<AsyncHttp>>
{
  Client& _;
  My::log::Logger mLogger;
  std::function<void(BoostResult<Response>&&)> mCb;

  _Client(Client& _,
          const char* loggerName,
          std::function<void(BoostResult<Response>&&)> cb)
    : _(_)
    , mLogger(loggerName, this)
    , mCb(std::move(cb))
  {
  }

  void exec(Request&& req) { mReq = std::move(req), do_request(); }

private:
  Conn mConn;
  std::uint32_t mRetry{ 0 };

  Request mReq;
  bb::flat_buffer mBuf;
  Response mRes;

  void do_request()
  {
    mConn = _.mConnPool.take();
    if (!mConn) {
      mConn = std::make_shared<Client::Connection>(_.mEx);
      mConn->mResolver.async_resolve(
        _.mConfig.mHost,
        _.mConfig.mPort,
        [&, self = shared_from_this()](auto&& a, auto&& b) {
          on_resolve(a, b);
        });
      return;
    }

    // TODO how to timeout?

    http::async_write(
      mConn->mSocket, mReq, [self = shared_from_this()](auto&& a, auto&& b) {
        self->on_write(a, b);
      });
  }

  void on_resolve(const BoostEC& ec,
                  const ba::ip::tcp::resolver::results_type& results) noexcept
  {
    if (ec) {
      BOOST_LOG_SEV(mLogger, warn) << "resolve failed: " << ec.message();
      return;
    }

    mConn->mSocket.async_connect(
      *results, [&, self = shared_from_this()](auto&& a) mutable {
        self->on_connect(a);
      });
  }

  void on_connect(const BoostEC& ec) noexcept
  {
    if (ec) {
      if (mRetry >= _.mConfig.mMaxRetry) {
        BOOST_LOG_SEV(mLogger, warn) << "connect failed: " << ec.message();
        return;
      }

      BOOST_LOG_SEV(mLogger, info) << "connect failed: " << ec.message()
                                   << ", retrying(" << ++mRetry << ")...";
      do_request();
      return;
    }

    http::async_write(
      mConn->mSocket, mReq, [self = shared_from_this()](auto&& a, auto&& b) {
        self->on_write(a, b);
      });
  }

  void on_write(const BoostEC& ec, std::size_t len) noexcept
  {
    if (ec) {
      if (mRetry >= _.mConfig.mMaxRetry) {
        BOOST_LOG_SEV(mLogger, warn) << "write failed: " << ec.message();
        return;
      }

      BOOST_LOG_SEV(mLogger, info) << "write failed: " << ec.message()
                                   << ", retrying(" << ++mRetry << ")...";
      do_request();
      return;
    }

    http::async_read(
      mConn->mSocket,
      mBuf,
      mRes,
      [self = shared_from_this()](auto&& a, auto&& b) { self->on_read(a, b); });
  }

  void on_read(const BoostEC& ec, std::size_t len) noexcept
  {
    if (ec) {
      BOOST_LOG_SEV(mLogger, warn) << "read failed: " << ec.message();
      return;
      // 如果读取响应失败，就不能再重试了，因为数据已经发出了，服务器状态可能已经改变。
    }

    auto keepAlive = mRes.keep_alive();
    mCb(std::move(mRes));
    if (keepAlive) {
      _.mConnPool.give(std::move(mConn));
      return;
      // TODO 定时失效再取出
    }

    {
      BoostEC ec;
      mConn->mSocket.shutdown(Socket::shutdown_both, ec);
      if (ec)
        BOOST_LOG_SEV(mLogger, warn) << "shutdown failed: " << ec.message();
      mConn->mSocket.close(ec);
      if (ec)
        BOOST_LOG_SEV(mLogger, warn) << "close failed: " << ec.message();
      // 这里显式地优雅关闭套接字，在出错情况下，boost::asio::ip::tcp::socket
      // 的析构函数会自动关闭套接字，不用担心资源泄漏的问题。
    }
  }
};

} // namespace

BoostResult<Response>
Client::http(const Request& req) noexcept {
  // TODO
};

void
Client::async_http(Request req, std::function<void(BoostResult<Response>&&)> cb)
{
  ba::post(mEx,
           [x = std::make_shared<_Client<AsyncHttp>>(
              *this, mLogName.c_str(), std::move(cb)),
            req = std::move(req)]() mutable { x->exec(std::move(req)); });
};

} // namespace MyHttp
