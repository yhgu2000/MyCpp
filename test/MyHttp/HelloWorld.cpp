#include "testutil.hpp"

#include <MyHttp/Client.hpp>
#include <MyHttp/HttpHelloWorld.hpp>

using namespace My;
using namespace MyHttp;

struct GlobalFixture
{
  static void setup() { init_loglevel(0); }
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);

BOOST_AUTO_TEST_CASE(basic)
{
  MyHttp::util::ThreadsExecutor ex(1);
  ex.start();

  HttpHelloWorld::Config serverConfig;
  HttpHelloWorld::Server server(serverConfig, ex);

  Endpoint ep(ba::ip::address_v4::loopback(), 8000);
  BOOST_REQUIRE(!server.start(ep));
  std::this_thread::sleep_for(100ms); // 等待服务器启动

  Client::Config clientConfig;
  clientConfig.mHost = "127.0.0.1";
  clientConfig.mPort = "8000";
  Client client(clientConfig, ex);

  Request req;
  req.version(11);
  req.method(bb::http::verb::get);
  req.target("/");
  req.set(bb::http::field::host, "127.0.0.1:8000");
  auto res = client.http(req);
  BOOST_REQUIRE(res);
  BOOST_TEST(res->result() == bb::http::status::ok);
  BOOST_TEST(bool(res->body() == "Hello, World!"_b));
}
