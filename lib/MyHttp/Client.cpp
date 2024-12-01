#include "Client.hpp"

#include <My/err.hpp>
#include <My/util.hpp>

using namespace My::util;
using namespace My::log;

namespace MyHttp {

struct Client::Connection
{
  std::atomic<Connection*> mNext;
  std::atomic<Connection*> mPrev;
  Socket mSocket;
  ba::steady_timer mTimer;
};

BoostResult<Response>
Client::http(Request& req) noexcept {
  // TODO
};

void
Client::async_http(Request& req,
                   std::function<void(BoostResult<Response>&&)> cb) {};

} // namespace MyHttp
