#define MYHTTP_CLIENT_PRIVATE public:
#include "Client.hpp"

#include <My/err.hpp>
#include <My/util.hpp>

using namespace My::util;
using namespace My::log;

namespace MyHttp {

namespace {

using Conn = std::shared_ptr<Client::Connection>;
using StdHRC = std::chrono::high_resolution_clock;

void
close_gracefully(Socket& socket, My::log::Logger& logger)
{
  BoostEC ec;
  socket.shutdown(Socket::shutdown_both, ec);
  if (ec)
    BOOST_LOG_SEV(logger, noti) << "shutdown failed: " << ec.message();
  socket.close(ec);
  if (ec)
    BOOST_LOG_SEV(logger, noti) << "close failed: " << ec.message();
  // 这里显式地优雅关闭套接字，在出错情况下，boost::asio::ip::tcp::socket
  // 的析构函数会自动关闭套接字，不用担心资源泄漏的问题。
}

std::pair<std::uint32_t, std::uint32_t>
parse_keep_alive(std::string_view value)
{
  std::uint32_t timeout = 0, max = 0;

  auto begin = value.find("timeout=");
  if (begin != std::string::npos) {
    begin += 8;
    auto end = value.find(',', begin);
    if (end == std::string::npos)
      end = value.size();
    timeout = std::stoul(std::string(value.substr(begin, end - begin)));
  }

  begin = value.find("max=");
  if (begin != std::string::npos) {
    begin += 4;
    auto end = value.find(',', begin);
    if (end == std::string::npos)
      end = value.size();
    max = std::stoul(std::string(value.substr(begin, end - begin)));
  }

  return { timeout, max };
}

void
handle_keep_alive(Conn conn,
                  Response& res,
                  Client& client,
                  My::log::Logger& logger)
{
  if (!res.keep_alive()) {
    close_gracefully(conn->mSocket, logger);
    return;
  }

  ba::steady_timer::duration timeout;
  auto keepAliveValue = res[http::field::keep_alive];
  if (keepAliveValue.empty()) {
    timeout = client.mConfig.mKeepAliveTimeout;
  } else {
    auto [t, m] = parse_keep_alive(keepAliveValue);
    if (t == 0 || m == 0) {
      close_gracefully(conn->mSocket, logger);
      return;
    }
    timeout = std::chrono::seconds(t);
  }

  // 将连接放回连接池，并设定计时器等过了有效时间将其丢弃。
  client.mConnPool.give(conn);
  conn->mTimer.expires_after(timeout);
  conn->mTimer.async_wait([conn = std::move(conn)](auto&& ec) {
    if (!ec)
      Client::Connection::Pool::drop(*conn);
    // 如果是这种情况，我们就不要优雅关闭连接了，因为远端可能已经关闭过了。
  });
}

struct AsyncHttp : std::enable_shared_from_this<AsyncHttp>
{
  Client& _;
  My::log::Logger mLogger;
  std::function<void(BoostResult<Response>&&)> mCb;

  AsyncHttp(Client& _,
            std::string logName,
            std::function<void(BoostResult<Response>&&)> cb)
    : _(_)
    , mLogger(std::move(logName), this)
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

  StdHRC::time_point mTiming, mTimingTotal;

  void do_request()
  {
    mConn = _.mConnPool.take();
    if (!mConn) {
      BOOST_LOG_SEV(mLogger, verb) << "resolving";
      mTimingTotal = mTiming = StdHRC::now();

      mConn = std::make_shared<Client::Connection>(_.mEx);
      mConn->mResolver.async_resolve(
        _.mConfig.mHost,
        _.mConfig.mPort,
        [self = shared_from_this()](auto&& a, auto&& b) {
          self->mConn->mTimer.cancel();
          self->on_resolve(a, b);
        });
      mConn->mTimer.async_wait([self = shared_from_this()](auto&& ec) {
        if (!ec)
          self->mConn->mResolver.cancel();
      });
      mConn->mTimer.expires_after(_.mConfig.mTimeout);
      return;
    }
    mConn->mTimer.cancel(); // 计时器可能正等着从连接池中移除连接，重置它。

    do_write();
  }

  void on_resolve(const BoostEC& ec,
                  const ba::ip::tcp::resolver::results_type& results) noexcept
  {
    if (ec) {
      BOOST_LOG_SEV(mLogger, noti) << "resolve failed: " << ec.message();
      return;
    }
    BOOST_LOG_SEV(mLogger, verb)
      << "resolved: " << results->endpoint().address() << ':'
      << results->endpoint().port() << " ("
      << to_string(StdHRC::now() - mTiming) << ')';

    BOOST_LOG_SEV(mLogger, verb) << "connecting";
    mTiming = StdHRC::now();
    mConn->mSocket.async_connect(*results,
                                 [self = shared_from_this()](auto&& a) mutable {
                                   self->mConn->mTimer.cancel();
                                   self->on_connect(a);
                                 });
    mConn->mTimer.async_wait([self = shared_from_this()](auto&& ec) {
      if (!ec) {
        BoostEC ec(ba::error::timed_out);
        self->mConn->mSocket.close(ec);
      }
    });
    mConn->mTimer.expires_after(_.mConfig.mTimeout);
  }

  void on_connect(const BoostEC& ec) noexcept
  {
    if (ec) {
      if (mRetry >= _.mConfig.mMaxRetry) {
        BOOST_LOG_SEV(mLogger, noti) << "connect failed: " << ec.message();
        return;
      }

      BOOST_LOG_SEV(mLogger, info) << "connect failed: " << ec.message()
                                   << ", retrying(" << ++mRetry << ")...";
      do_request();
      return;
    }
    BOOST_LOG_SEV(mLogger, verb)
      << "connected: " << mConn->mSocket.remote_endpoint() << " ("
      << to_string(StdHRC::now() - mTiming) << ')';

    do_write();
  }

  void do_write() noexcept
  {
    BOOST_LOG_SEV(mLogger, verb) << "writing";
    mTiming = StdHRC::now();

    http::async_write(
      mConn->mSocket, mReq, [self = shared_from_this()](auto&& a, auto&& b) {
        self->mConn->mTimer.cancel();
        self->on_write(a, b);
      });
    mConn->mTimer.async_wait([self = shared_from_this()](auto&& ec) {
      if (!ec) {
        BoostEC ec(ba::error::timed_out);
        self->mConn->mSocket.close(ec);
      }
    });
    mConn->mTimer.expires_after(_.mConfig.mTimeout);
  }

  void on_write(const BoostEC& ec, std::size_t len) noexcept
  {
    if (ec) {
      if (mRetry >= _.mConfig.mMaxRetry) {
        BOOST_LOG_SEV(mLogger, noti) << "write failed: " << ec.message();
        return;
      }

      BOOST_LOG_SEV(mLogger, info) << "write failed: " << ec.message()
                                   << ", retrying(" << ++mRetry << ")...";
      do_request();
      return;
    }
    BOOST_LOG_SEV(mLogger, verb) << "written: " << len << " bytes ("
                                 << to_string(StdHRC::now() - mTiming) << ')';

    BOOST_LOG_SEV(mLogger, verb) << "reading";
    mTiming = StdHRC::now();
    http::async_read(mConn->mSocket,
                     mBuf,
                     mRes,
                     [self = shared_from_this()](auto&& a, auto&& b) {
                       self->mConn->mTimer.cancel();
                       self->on_read(a, b);
                     });
    mConn->mTimer.async_wait([self = shared_from_this()](auto&& ec) {
      if (!ec) {
        BoostEC ec(ba::error::timed_out);
        self->mConn->mSocket.close(ec);
      }
    });
    mConn->mTimer.expires_after(_.mConfig.mTimeout);
  }

  void on_read(const BoostEC& ec, std::size_t len) noexcept
  {
    if (ec) {
      BOOST_LOG_SEV(mLogger, noti) << "read failed: " << ec.message();
      return;
      // 如果读取响应失败，就不能再重试了，因为数据已经发出了，服务器状态可能已经改变。
    }
    auto now = StdHRC::now();
    BOOST_LOG_SEV(mLogger, verb)
      << "read: " << len << " bytes (" << to_string(now - mTiming) << ", total "
      << to_string(now - mTimingTotal) << ')';

    handle_keep_alive(mConn, mRes, _, mLogger);
    mCb(std::move(mRes));
  }
};

} // namespace

BoostResult<Response>
Client::http(Request& req) noexcept
{
  req.prepare_payload();

  My::log::Logger logger(mLogName, this);
  StdHRC::time_point timingTotal, timing;
  timingTotal = StdHRC::now();

  BoostEC ec;
  auto conn = mConnPool.take();
  if (!conn) {
    conn = std::make_shared<Connection>(mEx);

    BOOST_LOG_SEV(logger, verb) << "resolving";
    timing = StdHRC::now();
    conn->mTimer.async_wait([&conn = *conn](auto&& ec) {
      if (!ec)
        conn.mResolver.cancel();
    });
    conn->mTimer.expires_after(mConfig.mTimeout);
    auto results = conn->mResolver.resolve(mConfig.mHost, mConfig.mPort, ec);
    if (ec) {
      BOOST_LOG_SEV(logger, noti) << "resolve failed: " << ec.message();
      return ec;
    }
    conn->mTimer.cancel();
    BOOST_LOG_SEV(logger, verb) << "resolved: " << results->endpoint().address()
                                << ':' << results->endpoint().port() << " ("
                                << to_string(StdHRC::now() - timing) << ')';

    BOOST_LOG_SEV(logger, verb) << "connecting";
    timing = StdHRC::now();
    conn->mTimer.async_wait([&conn = *conn](auto&& ec) {
      if (!ec) {
        BoostEC ec(ba::error::timed_out);
        conn.mSocket.close(ec);
      }
    });
    conn->mTimer.expires_after(mConfig.mTimeout);
    conn->mSocket.connect(*results, ec);
    if (ec) {
      BOOST_LOG_SEV(logger, noti) << "connect failed: " << ec.message();
      return ec;
    }
    conn->mTimer.cancel();
    BOOST_LOG_SEV(logger, verb)
      << "connected: " << conn->mSocket.remote_endpoint() << " ("
      << to_string(StdHRC::now() - timing) << ')';
  }

  BOOST_LOG_SEV(logger, verb) << "writing";
  timing = StdHRC::now();
  conn->mTimer.async_wait([&conn = *conn](auto&& ec) {
    if (!ec) {
      BoostEC ec(ba::error::timed_out);
      conn.mSocket.close(ec);
    }
  });
  conn->mTimer.expires_after(mConfig.mTimeout);
  auto reqSize = http::write(conn->mSocket, req, ec);
  if (ec) {
    BOOST_LOG_SEV(logger, noti) << "write failed: " << ec.message();
    return ec;
  }
  conn->mTimer.cancel();
  BOOST_LOG_SEV(logger, verb) << "written: " << reqSize << " bytes ("
                              << to_string(StdHRC::now() - timing) << ')';

  BOOST_LOG_SEV(logger, verb) << "reading";
  timing = StdHRC::now();
  conn->mTimer.async_wait([&conn = *conn](auto&& ec) {
    if (!ec) {
      BoostEC ec(ba::error::timed_out);
      conn.mSocket.close(ec);
    }
  });
  conn->mTimer.expires_after(mConfig.mTimeout);
  bb::flat_buffer buf;
  Response res;
  auto resSize = http::read(conn->mSocket, buf, res, ec);
  if (ec) {
    BOOST_LOG_SEV(logger, noti) << "read failed: " << ec.message();
    return ec;
  }
  auto now = StdHRC::now();
  BOOST_LOG_SEV(logger, verb)
    << "read: " << resSize << " bytes (" << to_string(now - timing)
    << ", total " << to_string(now - timingTotal) << ')';

  handle_keep_alive(conn, res, *this, logger);
  return res;
};

void
Client::async_http(Request req, std::function<void(BoostResult<Response>&&)> cb)
{
  req.prepare_payload();
  ba::post(mEx,
           [x = std::make_shared<AsyncHttp>(*this, mLogName, std::move(cb)),
            req = std::move(req)]() mutable { x->exec(std::move(req)); });
};

bj::value
Client::Config::to_jval() const noexcept
{
  bj::object jobj;
  jobj.emplace("Host", mHost);
  jobj.emplace("Port", mPort);
  jobj.emplace("BufferLimit", mBufferLimit);
  jobj.emplace(
    "Timeout",
    std::chrono::duration_cast<std::chrono::milliseconds>(mTimeout).count());
  jobj.emplace("MaxRetry", mMaxRetry);
  jobj.emplace(
    "KeepAliveTimeout",
    std::chrono::duration_cast<std::chrono::milliseconds>(mKeepAliveTimeout)
      .count());
  return { std::move(jobj) };
}

void
Client::Config::jval_to(const bj::value& jval)
{
  auto& jobj = jval.as_object();
  mHost = jobj.at("Host").as_string();
  mPort = jobj.at("Port").as_string();
  mBufferLimit = jobj.at("BufferLimit").as_uint64();
  mTimeout = std::chrono::milliseconds(jobj.at("Timeout").as_uint64());
  mMaxRetry = jobj.at("MaxRetry").as_uint64();
  mKeepAliveTimeout =
    std::chrono::milliseconds(jobj.at("KeepAliveTimeout").as_uint64());
}

} // namespace MyHttp
