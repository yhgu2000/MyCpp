#include "Server.hpp"

#include <My/err.hpp>
#include <My/util.hpp>

using namespace My::util;
using namespace My::log;

namespace MyHttp {

Server::~Server() noexcept
{
  if (mIoCtx.stopped())
    return;

  std::atomic<bool> stopped = false;
  ba::post(mAcpt.get_executor(), [this, &stopped] {
    if (mAcpt.is_open()) {
      mAcpt.cancel(), mAcpt.close();
      BOOST_LOG_SEV(mLogger, noti) << "closed in destructor";
    }
    stopped.store(true, std::memory_order_release);
  });

  while (!stopped.load(std::memory_order_acquire))
    ;
}

void
Server::start(const Endpoint& endpoint)
{
  ba::post(mAcpt.get_executor(), [this, endpoint] {
    mAcpt.open(endpoint.protocol());
    mAcpt.set_option(ba::socket_base::reuse_address(true));
    mAcpt.bind(endpoint);
    mAcpt.listen();
    BOOST_LOG_SEV(mLogger, noti) << "started on " << endpoint;

    do_accept();
  });
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
  mAcpt.async_accept(
    mIoCtx, [this](auto&& a, auto&& b) { on_accept(a, std::move(b)); });
}

void
Server::on_accept(const BoostEC& ec, Socket&& sock)
{
  if (ec) {
    if (ec != ba::error::operation_aborted)
      BOOST_LOG_SEV(mLogger, crit) << "accept failed: " << ec.message();
    return;
  }
  BOOST_LOG_SEV(mLogger, verb) << "accepted " << sock.remote_endpoint();
  come(std::move(sock));
  do_accept();
}

} // namespace OFA
