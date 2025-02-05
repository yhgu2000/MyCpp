#include "project.h"
#include "timestamp.h"

static const char kVersionInfo[] =
  "MyHttp Server App\n"
  "=================\n"
  "A simple HTTP server based on the MyHttp library.\n"
  "\n"
  "Built: " MyCpp_TIMESTAMP "\n"
  "Version: 1.0\n"
  "Copyright (C) 2024-2025 Yuhao Gu. All Rights Reserved.";

#include <My/err.hpp>
#include <My/log.hpp>
#include <My/util.hpp>

using namespace My::util;

// ========================================================================== //
// 日志配置
// ========================================================================== //

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace {

int gLogLevel = My::log::Level::noti;

void
init_log()
{
  boost::log::add_common_attributes();
  boost::log::add_console_log(std::clog,
                              boost::log::keywords::format = &My::log::format);
  boost::log::core::get()->set_filter(
    [](const boost::log::attribute_value_set& attrs) {
      return attrs["Severity"].extract<My::log::Level>() >= gLogLevel;
    });
}

} // namespace

// ========================================================================== //
// 服务器
// ========================================================================== //

#include <MyHttp/ServerBuilder.hpp>
#include <thread>

namespace {

int gThreads = std::thread::hardware_concurrency();
std::string gManifest;

MyHttp::util::ThreadsExecutor* gExecutor;
MyHttp::ServerBuilder::Servers gServers;

void
stop_servers(int sig)
{
  MyHttp::ServerBuilder::stop_all(gServers);
  // gExecutor->stop();
}

void
build_and_run_servers()
{
  MyHttp::util::ThreadsExecutor ex(gThreads);
  gExecutor = &ex;
  ex.start();

  MyHttp::ServerBuilder sb(ex);
  sb.register_builtins();
  gServers = sb.build_json_file(gManifest);
  MyHttp::ServerBuilder::start_all(gServers);

  signal(SIGINT, &stop_servers);
  signal(SIGTERM, &stop_servers);
  ex.wait();
}

} // namespace

// ========================================================================== //
// 配置示例
// ========================================================================== //

namespace {

const char kExampleManifest[] = R"(
{
  "hello-world": {
    "Type": "HttpHelloWorld",
    "Host": "0.0.0.0",
    "Port": 8001,
    "Backlog": 128,
    "Details": {
      "BufferLimit": 8192,
      "KeepAliveTimeout": 3,
      "KeepAliveMax": 1,
    },
  },
  "matpowsum": {
    "Type": "HttpMatpowsum",
    "Host": "127.0.0.1",
    "Port": 8002,
    "Backlog": 4096,
    "Details": {
      "BufferLimit": 8192,
      "KeepAliveTimeout": 3,
      "KeepAliveMax": null,
    },
  },
}
)";

} // namespace

// ========================================================================== //
// 主函数
// ========================================================================== //

#include "po.hpp"

int
main(int argc, char* argv[])
try {
  po::options_description od("Options");
  od.add_options()                                                 //
    ("version,v", "print version info")                            //
    ("help,h", "print help info")                                  //
    ("log,l", povd(gLogLevel), "log level")                        //
    ("threads,t", povd(gThreads), "number of threads")             //
    ("manifest-example", "print example of service manifest file") //
    ("manifest", povd(gManifest), "path to service manifest file") //
    ;

  po::positional_options_description pod;
  pod.add("manifest", 1);

  po::variables_map vmap;
  po::store(
    po::command_line_parser(argc, argv).options(od).positional(pod).run(),
    vmap);
  if (vmap.count("help") || argc == 1) {
    std::cout << od << std::endl;
    return 0;
  }
  if (vmap.count("version")) {
    std::cout << kVersionInfo << std::endl;
    return 0;
  }
  if (vmap.count("manifest-example")) {
    std::cout << kExampleManifest << std::endl;
    return 0;
  }
  po::notify(vmap);

  init_log();
  build_and_run_servers();
}

catch (My::Err& e) {
  std::cout << e.what() << ": " << e.info() << std::endl;
  return -3;
}

catch (std::exception& e) {
  std::cout << "Exception: " << e.what() << std::endl;
  return -2;
}

catch (...) {
  std::cout << "UNKNOWN EXCEPTION" << std::endl;
  return -1;
}
