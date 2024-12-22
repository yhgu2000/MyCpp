#include "testutil.hpp"

#include <MyHttp/Client.hpp>
#include <MyHttp/HttpHelloWorld.hpp>

using namespace My;
using namespace MyHttp;

struct GlobalFixture
{
  static void setup() { init_loglevel(); }
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);

BOOST_AUTO_TEST_CASE(basic)
{
  reset_loglevel(My::log::verb);

  MyHttp::util::ThreadsExecutor ex(1);
  ex.start();

  HttpHelloWorld::Server server(ex);
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

  std::atomic<bool> done(false);
  client.async_http(req, [&](auto&& res) {
    BOOST_REQUIRE(res);
    BOOST_TEST(res->result() == bb::http::status::ok);
    BOOST_TEST(bool(res->body() == "Hello, World!"_b));
    done = true;
  });
  while (!done)
    std::this_thread::sleep_for(100ms);

  client.clear_connections();
  server.stop();
  ex.wait();
}

BOOST_AUTO_TEST_CASE(stress)
{
  reset_loglevel(My::log::warn);

  auto loopsEnv = std::getenv("LOOPS");
  auto loops = loopsEnv ? std::atoi(loopsEnv) : 1000;
  auto threadsEnv = std::getenv("THREADS");
  auto threadsNum =
    threadsEnv ? std::atoi(threadsEnv) : std::thread::hardware_concurrency();

  MyHttp::util::ThreadsExecutor ex(threadsNum, "ex");
  MyHttp::util::ThreadsExecutor acptEx(1, "acptEx");
  ex.start(), acptEx.start();
  HttpHelloWorld::Server server(ex, acptEx);

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

  std::atomic<unsigned> failure(0);
  auto timingBegin = std::chrono::high_resolution_clock::now();
  for (unsigned i = 0; i < loops; ++i)
    ba::post(ex.mIoCtx, [&] {
      for (unsigned t = 0; t < threadsNum; ++t) {
        client.async_http(req, [&](auto&& res) {
          if (!res || res->result() == bb::http::status::ok)
            failure.fetch_add(1, std::memory_order_relaxed);
        });
      }
    });
  std::cout << "request posted ("
            << (std::chrono::high_resolution_clock::now() - timingBegin) << ")"
            << std::endl;
  ex.wait();
  auto timingEnd = std::chrono::high_resolution_clock::now();
  std::cout << "requests handled (" << (timingEnd - timingBegin) << ")"
            << std::endl;
  auto ns = (timingEnd - timingBegin).count();

  std::cout << threadsNum << " threads perform " << loops << " loops ("
            << failure << " failure), with total "
            << loops * threadsNum / (double(ns) / 1e9)
            << " throughput per second" << std::endl;

  /**
   * 我发现一个很有趣的现象：吞吐量并不是呈现屋顶线的形状，随着 loops
   * 的增长，在 1000 附近会很高，而过大或过小的吞吐量都比较低 ——
   * 这对于服务器来说不是一个好的表现，这意味着服务器在高负载情况下可能会
   * 出现毁灭性的全面崩溃恶性循环。
   */
}
