#include "Client.hpp"

#include <My/err.hpp>
#include <My/util.hpp>

using namespace My::util;
using namespace My::log;

namespace MyHttp {

namespace {

using Conn = std::shared_ptr<Client::Connection>;

// 续程类？续程类！
struct ResolveAndConnect : std::enable_shared_from_this<ResolveAndConnect>
{
  ba::io_context& mIoCtx;
  std::function<void(Conn)> mCb;

  void exec(My::log::LoggerMt& logger,
            ba::ip::tcp::resolver& resolver,
            const std::string& host,
            const std::string& port)
  {
    assert(conn);
    resolver.async_resolve(
      host, port, [&, self = shared_from_this()](auto&& a, auto&& b) {
        on_resolve(logger, a, std::move(b));
      });
  }

  void on_resolve(My::log::LoggerMt& logger,
                  const BoostEC& ec,
                  ba::ip::tcp::resolver::results_type results)
  {
    if (ec) {
      BOOST_LOG_SEV(logger, warn) << "resolve failed: " << ec.message();
      return;
    }

    auto conn = std::make_shared<Client::Connection>(mIoCtx);
    conn->mSocket.async_connect(
      *results, [&, conn, self = shared_from_this()](auto&& a) mutable {
        on_connect(a, std::move(conn));
      });
  }

  void on_connect(const BoostEC& ec, Conn conn)
  {
    if (ec) {
      BOOST_LOG_SEV(conn->mLogger, warn) << "connect failed: " << ec.message();
      return;
    }
    mCb(std::move(conn));
  }
};

struct RequestAndResponse : std::enable_shared_from_this<RequestAndResponse>
{
  std::function<void(BoostResult<Response>&&)> mCb;

  Conn mConn;

  void exec(Conn conn, Request req)
  {
    assert(conn);
    mConn = std::move(conn);
    // TOOD
  }
};

} // namespace

BoostResult<Response>
Client::http(const Request& req) noexcept {
  // TODO
};

void
Client::async_http(Request req,
                   std::function<void(BoostResult<Response>&&)> cb) {
  // TODO
};

} // namespace MyHttp
