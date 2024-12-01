#include "servers.hpp"
#include "HttpHelloWorld.hpp"
#include "HttpMatpowsum.hpp"

namespace MyHttp::servers {

void
HelloWorld::come(Socket&& sock)
{
  auto handler = std::make_shared<HttpHelloWorld>(std::move(sock), mConfig);
  handler->start();
}

void
Matpowsum::come(Socket&& sock)
{
  auto handler = std::make_shared<HttpMatpowsum>(std::move(sock), mConfig);
  handler->start();
}

} // namespace MyHttp::servers
