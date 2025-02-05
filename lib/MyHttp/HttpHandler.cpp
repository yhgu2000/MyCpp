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
  mStream.cancel(), mStream.close();
}

void
HttpHandler::on_handle(std::exception_ptr eptr) noexcept
{
  auto timingEnd = std::chrono::high_resolution_clock::now();

  std::string errstr;
  if (eptr) {
    try {
      std::rethrow_exception(std::move(eptr));
    } catch (My::Err& e) {
      errstr = "\n"s + e.what() + " | " + e.info();
    } catch (std::exception& e) {
      errstr = "\n"s + e.what();
    } catch (...) {
      errstr = "\nUNKNOWN ERROR";
    }
    mResponse.result(http::status::internal_server_error);
    mResponse.clear();
    mResponse.body() = to_bytes(errstr);
  }

  BOOST_LOG_SEV(mLogger, errstr.empty() ? verb : info)
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
    else if (ec == bb::error::timeout || ec == ba::error::operation_aborted)
      BOOST_LOG_SEV(mLogger, verb) << "read timeout";
    else
      BOOST_LOG_SEV(mLogger, info) << "read failed: " << ec.message();
    return;
  }

  mTimingHandleBegin = std::chrono::high_resolution_clock::now();
  do_handle();
}

void
HttpHandler::do_write()
{
  mResponse.version(11);

  if (!mRequest.keep_alive()) {
    mResponse.keep_alive(false);
  } else if (mConfig.mKeepAliveMax == UINT32_MAX) {
    mResponse.keep_alive(true);
  } else if (mKeepAliveCount <= mConfig.mKeepAliveMax) {
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
    BOOST_LOG_SEV(mLogger, info) << "write failed: " << ec.message();
    return;
  }

  ++mKeepAliveCount;
  if (mConfig.mKeepAliveMax == UINT32_MAX ||
      mKeepAliveCount < mConfig.mKeepAliveMax) {
    do_read();
  } else {
    do_close("finished");
  }
}

void
HttpHandler::do_close(const char* reason)
{
  BoostEC ec;
  mStream.socket().shutdown(ba::ip::tcp::socket::shutdown_both, ec);
  if (ec) {
    BOOST_LOG_SEV(mLogger, info) << "shutdown failed: " << ec.message();
    return;
  }

  auto timingEnd = std::chrono::high_resolution_clock::now();
  BOOST_LOG_SEV(mLogger, verb)
    << "done: " << reason << " (" << to_string(timingEnd - mTimingBegin) << ")";
}

bj::value
HttpHandler::Config::to_jval() const noexcept
{
  bj::object jobj;
  jobj.emplace("BufferLimit", mBufferLimit);
  jobj.emplace("KeepAliveTimeout", mKeepAliveTimeout);
  if (mKeepAliveMax != UINT32_MAX)
    jobj.emplace("KeepAliveMax", mKeepAliveMax);
  else
    jobj.emplace("KeepAliveMax", nullptr);
  return { std::move(jobj) };
}

void
HttpHandler::Config::jval_to(const bj::value& jval)
{
  auto& jobj = jval.as_object();
  mBufferLimit = jobj.at("BufferLimit").as_int64();
  mKeepAliveTimeout = jobj.at("KeepAliveTimeout").as_int64();
  auto& keepAliveMax = jobj.at("KeepAliveMax");
  if (keepAliveMax.is_null())
    mKeepAliveMax = UINT32_MAX;
  else
    mKeepAliveMax = keepAliveMax.as_int64();
}

} // namespace MyHttp
