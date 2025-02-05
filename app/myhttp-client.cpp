// #include "project.h"
// #include "timestamp.h"

// static const char kVersionInfo[] =
//   "MyHttp Client App\n"
//   "=======================\n"
//   "A simple CLI tool to send HTTP requests based on the MyHttp library.\n"
//   "\n"
//   "Built: " MyCpp_TIMESTAMP "\n"
//   "Version: 1.0\n"
//   "Copyright (C) 2024-2025 Yuhao Gu. All Rights Reserved.";

// #include <My/err.hpp>
// #include <My/log.hpp>
// #include <My/util.hpp>

// using namespace My::util;

// // ========================================================================== //
// // 配置日志
// // ========================================================================== //

// #include <boost/log/utility/setup/common_attributes.hpp>
// #include <boost/log/utility/setup/console.hpp>

// namespace {

// int gLogLevel = My::log::Level::noti;

// void
// init_log()
// {
//   boost::log::add_common_attributes();
//   boost::log::add_console_log(std::clog,
//                               boost::log::keywords::format = &My::log::format);
//   boost::log::core::get()->set_filter(
//     [](const boost::log::attribute_value_set& attrs) {
//       return attrs["Severity"].extract<My::log::Level>() >= gLogLevel;
//     });
// }

// } // namespace

// // ========================================================================== //
// // 主函数
// // ========================================================================== //

// #include "po.hpp"

// int
// main(int argc, char* argv[])
// try {
//   po::options_description od("Options");
//   od.add_options()                          //
//     ("version,v", "print version info")     //
//     ("help,h", "print help info")           //
//     ("log", povd(gLogLevel), "log level")   //
//     ("...",                                 //
//      po::value<std::vector<std::string>>(), //
//      "other arguments")                     //
//     ;

//   po::positional_options_description pod;
//   pod.add("...", -1);

//   std::vector<std::string> opts{ argv[0] };
//   for (int i = 1; i < argc; ++i) {
//     if (argv[i][0] == '-')
//       opts.emplace_back(argv[i]);
//     else
//       break;
//   }

//   po::variables_map vmap;
//   auto parsed = po::command_line_parser(opts)
//                   .options(od)
//                   .positional(pod)
//                   .allow_unregistered()
//                   .run();
//   po::store(parsed, vmap);

//   if (vmap.count("help") || argc == 1) {
//     std::cout << od
//               << "\n"
//                  "Sub Commands:\n";
//     for (auto&& i : kSubCmds)
//       std::cout << "  " << std::left << std::setw(12) << i.mName << i.mInfo
//                 << '\n';
//     std::cout << "\n"
//                  "[HINT: use '<subcmd> --help' to get help for sub commands.]\n"
//               << std::endl;
//     return 0;
//   }

//   if (vmap.count("version")) {
//     std::cout << kVersionInfo << std::endl;
//     return 0;
//   }

//   // 如果必须的选项没有指定，在这里会发生错误，因此 version 和 help
//   // 选项要放在前面。
//   po::notify(vmap);

//   if (opts.size() < argc) {
//     std::string cmd = argv[opts.size()];
//     for (auto&& i : kSubCmds) {
//       if (cmd == i.mName)
//         return i.mFunc(argc - opts.size(), argv + opts.size());
//     }
//     std::cout << "invalid sub command '" << cmd << "'." << std::endl;
//     return 1;
//   }
// }

// catch (My::Err& e) {
//   std::cout << e.what() << ": " << e.info() << std::endl;
//   return -3;
// }

// catch (std::exception& e) {
//   std::cout << "Exception: " << e.what() << std::endl;
//   return -2;
// }

// catch (...) {
//   std::cout << "UNKNOWN EXCEPTION" << std::endl;
//   return -1;
// }
