#include "project.h"
#include "timestamp.h"

static const char kVersionInfo[] =
  "CLI Example\n"
  "\n"
  "Built: " MyCpp_TIMESTAMP "\n"
  "Project: " MyCpp_VERSION "\n"
  "Copyright (C) 2023 Yuhao Gu. All Rights Reserved.";

#include "po.hpp"

#include <array>
#include <iomanip>
#include <iostream>

#include <My/err.hpp>
#include <My/log.hpp>
#include <My/util.hpp>

using namespace My::util;

// ========================================================================== //
// 子命令示例
// ========================================================================== //

namespace {

int
subcmd(int argc, char* argv[])
{
  po::options_description od("'subcmd' Options");
  od.add_options()                                        //
    ("help,h", "print help info")                         //
    ("output,o", po::value<std::string>(), "output path") //
    ;

  po::positional_options_description pod;
  pod.add("output", 1);

  po::variables_map vmap;
  po::store(
    po::command_line_parser(argc, argv).options(od).positional(pod).run(),
    vmap);
  po::notify(vmap);

  if (vmap.count("help") || argc == 1) {
    std::cout << od << std::endl;
    return 0;
  }

  std::cout << "output path is '" << vmap["output"].as<std::string>() << "'\n"
            << std::flush;

  return 0;
}

} // namespace

// ========================================================================== //
// 配置日志
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
// 主函数
// ========================================================================== //

namespace {

struct SubCmd
{
  const char *mName, *mInfo;
  int (*mFunc)(int argc, char* argv[]);
};

const SubCmd kSubCmds[] = {
  { "subcmd", "subcmd example", &subcmd },
  // TODO
};

} // namespace

int
main(int argc, char* argv[])
try {
  po::options_description od("Options");
  od.add_options()                          //
    ("version,v", "print version info")     //
    ("help,h", "print help info")           //
    ("log", povd(gLogLevel), "log level")   //
    ("...",                                 //
     po::value<std::vector<std::string>>(), //
     "other arguments")                     //
    ;

  po::positional_options_description pod;
  pod.add("...", -1);

  std::vector<std::string> opts{ argv[0] };
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-')
      opts.emplace_back(argv[i]);
    else
      break;
  }

  po::variables_map vmap;
  auto parsed = po::command_line_parser(opts)
                  .options(od)
                  .positional(pod)
                  .allow_unregistered()
                  .run();
  po::store(parsed, vmap);

  if (vmap.count("help") || argc == 1) {
    std::cout << od
              << "\n"
                 "Sub Commands:\n";
    for (auto&& i : kSubCmds)
      std::cout << "  " << std::left << std::setw(12) << i.mName << i.mInfo
                << '\n';
    std::cout << "\n"
                 "[HINT: use '<subcmd> --help' to get help for sub commands.]\n"
              << std::endl;
    return 0;
  }

  if (vmap.count("version")) {
    std::cout << kVersionInfo << std::endl;
    return 0;
  }

  // 如果必须的选项没有指定，在这里会发生错误，因此 version 和 help
  // 选项要放在前面。
  po::notify(vmap);

  if (opts.size() < argc) {
    std::string cmd = argv[opts.size()];
    for (auto&& i : kSubCmds) {
      if (cmd == i.mName)
        return i.mFunc(argc - opts.size(), argv + opts.size());
    }
    std::cout << "invalid sub command '" << cmd << "'." << std::endl;
    return 1;
  }
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
