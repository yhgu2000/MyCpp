#include "Server.hpp"

#include <My/err.hpp>
#include <My/util.hpp>

using namespace My::util;
using namespace My::log;

namespace MyHttp {

BoostEC
Server::start(const Endpoint& endpoint, int backlog)
{
  BoostEC ec;
  mAcpt.open(endpoint.protocol(), ec);
  if (ec) {
    BOOST_LOG_SEV(mLogger, info) << "open failed: " << ec.message();
    return ec;
  }

  mAcpt.set_option(ba::socket_base::reuse_address(true), ec);
  if (ec) {
    BOOST_LOG_SEV(mLogger, info) << "set_option failed: " << ec.message();
    return ec;
  }

  mAcpt.bind(endpoint, ec);
  if (ec) {
    BOOST_LOG_SEV(mLogger, info) << "bind failed: " << ec.message();
    return ec;
  }

  mAcpt.listen(backlog, ec);
  if (ec) {
    BOOST_LOG_SEV(mLogger, info) << "listen failed: " << ec.message();
    return ec;
  }

  BOOST_LOG_SEV(mLogger, noti) << "started on " << endpoint;
  do_accept();
  return ec;
}

void
Server::stop()
{
  ba::post(mAcpt.get_executor(), [this] {
    mAcpt.cancel(), mAcpt.close();
    BOOST_LOG_SEV(mLogger, noti) << "stopped";
  });
}

void
Server::do_accept()
{
  mAcpt.async_accept(mAcpt.get_executor(), [this](auto&& a, auto&& b) {
    on_accept(a, std::move(b));
  });
}

void
Server::on_accept(const BoostEC& ec, Socket&& sock)
{
  if (ec) {
    if (ec != ba::error::operation_aborted)
      BOOST_LOG_SEV(mLogger, info) << "accept failed: " << ec.message();
    return;
  }
  BOOST_LOG_SEV(mLogger, verb) << "accepted " << sock.remote_endpoint();
  come(std::move(sock));
  do_accept();
}

} // namespace OFA
