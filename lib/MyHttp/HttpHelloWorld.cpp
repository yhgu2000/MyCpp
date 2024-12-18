#include "HttpHelloWorld.hpp"

#include <My/util.hpp>

using namespace My::util;

namespace MyHttp {

void
HttpHelloWorld::do_handle() noexcept
{
  mResponse.result(http::status::ok);
  mResponse.set(http::field::content_type, "text/plain");
  mResponse.body() = "Hello, World!"_b;
  on_handle(nullptr);
}

void
HttpHelloWorld::Server::come(Socket&& sock)
{
  std::make_shared<HttpHelloWorld>(std::move(sock), mConfig)->start();
}

} // namespace MyHttp
