#include "HttpHandler.hpp"

#include <My/err.hpp>
#include <My/util.hpp>

using namespace My::util;
using namespace My::log;

namespace MyHttp {

void
HttpHandler::start()
{
  BOOST_LOG_SEV(mLogger, verb) << "start: " << strsock(mStream.socket());
  mTimingBegin = std::chrono::high_resolution_clock::now();
  do_read();
}

void
HttpHandler::stop()
{
  BOOST_LOG_SEV(mLogger, verb) << "stop: " << strsock(mStream.socket());
  BoostEC ec;
  mStream.socket().cancel(ec);
  if (ec)
    BOOST_LOG_SEV(mLogger, warn) << "cancel failed: " << ec.message();
  mStream.socket().close(ec);
  if (ec)
    BOOST_LOG_SEV(mLogger, warn) << "close failed: " << ec.message();
}

void
HttpHandler::on_handle(std::exception_ptr eptr)
{
  auto timingEnd = std::chrono::high_resolution_clock::now();

  std::string errstr;
  if (eptr) {
    try {
      std::rethrow_exception(eptr);
    } catch (My::Err& e) {
      errstr = "\n"s + e.what() + " | " + e.info();
    } catch (std::exception& e) {
      errstr = "\n"s + e.what();
    }
    mResponse.result(http::status::internal_server_error);
    mResponse.clear();
    mResponse.body() = to_bytes(errstr);
  }

  BOOST_LOG_SEV(mLogger, errstr.empty() ? info : verb)
    << mRequest.method() << ' ' << mRequest.target() << " --" << mKeepAliveCount
    << "-> " << mResponse.result_int() << " : "
    << to_string(timingEnd - mTimingHandleBegin) << errstr;

  do_write();
}

void
HttpHandler::do_read()
{
  mRequest = {};
  mStream.expires_after(std::chrono::seconds(mConfig.mKeepAliveTimeout));
  http::async_read(
    mStream,
    mBuffer,
    mRequest,
    [self = shared_from_this()](auto&& a, auto&& b) { self->on_read(a, b); });
}

void
HttpHandler::on_read(const BoostEC& ec, std::size_t len)
{
  if (ec) {
    if (ec == http::error::end_of_stream)
      do_close("eof");
    else if (ec == bb::error::timeout)
      BOOST_LOG_SEV(mLogger, verb) << "read timeout";
    else
      BOOST_LOG_SEV(mLogger, warn) << "read failed: " << ec.message();
    return;
  }

  mTimingHandleBegin = std::chrono::high_resolution_clock::now();
  do_handle();
}

void
HttpHandler::do_write()
{
  mResponse.version(11);

  if (mRequest.keep_alive() && mKeepAliveCount < mConfig.mKeepAliveMax) {
    mResponse.keep_alive(true);
    mResponse.set(http::field::connection, "keep-alive");
    mResponse.set(http::field::keep_alive,
                  "timeout=" + to_string(mConfig.mKeepAliveTimeout) +
                    ", max=" + to_string(mConfig.mKeepAliveMax));
  } else {
    mResponse.keep_alive(false);
  }

  mResponse.set(http::field::server, "MyHttp");
  mResponse.prepare_payload();

  mStream.expires_never();
  http::async_write(
    mStream, mResponse, [self = shared_from_this()](auto&& a, auto&& b) {
      self->on_write(a, b);
    });
}

void
HttpHandler::on_write(const BoostEC& ec, std::size_t len)
{
  if (ec) {
    BOOST_LOG_SEV(mLogger, warn) << "write failed: " << ec.message();
    return;
  }

  if (mKeepAliveCount < mConfig.mKeepAliveMax) {
    ++mKeepAliveCount;
    do_read();
    return;
  }

  do_close("finished");
}

void
HttpHandler::do_close(const char* reason)
{
  BoostEC ec;
  mStream.socket().shutdown(ba::ip::tcp::socket::shutdown_both, ec);
  if (ec) {
    BOOST_LOG_SEV(mLogger, warn) << "shutdown failed: " << ec.message();
    return;
  }

  auto timingEnd = std::chrono::high_resolution_clock::now();
  BOOST_LOG_SEV(mLogger, verb)
    << "done: " << reason << " (" << to_string(timingEnd - mTimingBegin) << ")";
}

} // namespace MyHttp
