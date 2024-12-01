#include "project.h"
#include "timestamp.h"

static const char kVersionInfo[] =
  "My C/C++ Statistic Tool\n"
  "=======================\n"
  "This tool is to measure some important C/C++ statistics for guiding"
  "program design like the time to start a thread, the max throughput"
  "of a single mutex, etc.\n"
  "\n"
  "Built: " MyCpp_TIMESTAMP "\n"
  "Version: 1.0\n"
  "Copyright (C) 2024 Yuhao Gu. All Rights Reserved.";

#include "po.hpp"

#include <array>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include <My/SpinMutex.hpp>
#include <My/err.hpp>
#include <My/log.hpp>
#include <My/util.hpp>

using namespace My::util;
using HRC = std::chrono::high_resolution_clock;

// ========================================================================== //
// 线程创建速度
// ========================================================================== //

namespace {

int
create_threads(int argc, char* argv[])
{
  std::uint32_t t = 3;
  std::uint32_t n = 1000;
  std::uint32_t tn = 1;

  po::options_description od("'create_threads' Options");
  od.add_options()                                   //
    ("help,h", "print help info")                    //
    ("t", povd(t), "test times")                     //
    ("n", povd(n), "number of thread to create")     //
    ("tn", povd(tn), "thread num, 0 - use all cpus") //
    ;
  po::variables_map vmap;
  po::store(po::command_line_parser(argc, argv).options(od).run(), vmap);
  if (vmap.count("help")) {
    std::cout << od << std::endl;
    return 0;
  }
  po::notify(vmap);

  if (tn == 0)
    tn = std::thread::hardware_concurrency();
  std::cout << "creating " << n << " * " << tn << " threads." << std::endl;

  for (int _ = 0; _ < t; ++_) {
    auto timingStart = HRC::now();

    std::vector<std::thread> threads;
    for (std::uint32_t i = 0; i < tn; ++i) {
      threads.emplace_back([n] {
        for (std::uint32_t i = 0; i < n; ++i)
          std::thread(noop).join();
      });
    }
    for (auto&& t : threads)
      t.join();

    auto timingEnd = HRC::now();
    auto duration = timingEnd - timingStart;
    auto seconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() /
      1000.0;
    std::cout << _ << '\t' << duration << " (" << (double(n * tn) / seconds)
              << " /s, " << double(n) / seconds << " /s*tn)" << std::endl;
  }

  return 0;
}

} // namespace

// ========================================================================== //
// 各种锁的性能
// ========================================================================== //

namespace {

enum class LockType
{
  kMutex,
  kRecursive,
  kShared,
  // spin
  kSpin,
  kSpinRecursive,
  kSpinShared,
  // timed (TODO)
  kMutexTimed,
  kRecursiveTimed,
  kSharedTimed,
  // spin timed (TODO)
  kSpinTimed,
  kSpinRecursiveTimed,
  kSpinSharedTimed,
};

std::istream&
operator>>(std::istream& is, LockType& lt)
{
  std::string s;
  is >> s;
  if (s == "mutex" || s == "m")
    lt = LockType::kMutex;
  else if (s == "recursive" || s == "r")
    lt = LockType::kRecursive;
  else if (s == "shared" || s == "s")
    lt = LockType::kShared;
  else if (s == "spin" || s == "sm")
    lt = LockType::kSpin;
  else if (s == "spin_recursive" || s == "sr")
    lt = LockType::kSpinRecursive;
  else if (s == "spin_shared" || s == "ss")
    lt = LockType::kSpinShared;
  else
    is.setstate(std::ios_base::failbit);
  return is;
}

std::ostream&
operator<<(std::ostream& os, LockType lt)
{
  switch (lt) {
    case LockType::kMutex:
      return os << "mutex";
    case LockType::kRecursive:
      return os << "recursive";
    case LockType::kShared:
      return os << "shared";
    case LockType::kSpin:
      return os << "spin";
    case LockType::kSpinRecursive:
      return os << "spin_recursive";
    case LockType::kSpinShared:
      return os << "spin_shared";
  }
  return os;
}

int
lock_mutex(int argc, char* argv[])
{
  std::uint32_t t = 3;
  std::uint32_t n = 1000000;
  std::uint32_t tn = 1;
  LockType lt = LockType::kMutex;

  po::options_description od("'lock_mutex' Options");
  od.add_options()                                           //
    ("help,h", "print help info")                            //
    ("t", povd(t), "test times")                             //
    ("n", povd(n), "number of mutex lock-unlock operations") //
    ("tn", povd(tn), "thread num, 0 - use all cpus")         //
    ("lt", povd(lt), "lock type: [s]<m|r|s>[t]")             //
    ;
  po::variables_map vmap;
  po::store(po::command_line_parser(argc, argv).options(od).run(), vmap);
  if (vmap.count("help")) {
    std::cout << od << std::endl;
    return 0;
  }
  po::notify(vmap);

  if (tn == 0)
    tn = std::thread::hardware_concurrency();

  std::function<void()> work;
  switch (lt) {
    case LockType::kMutex: {
      auto m = std::make_shared<std::mutex>();
      work = [n, m] {
        std::lock_guard<std::mutex> lock(*m);
        noop();
      };
    } break;

    case LockType::kRecursive: {
      auto m = std::make_shared<std::recursive_mutex>();
      work = [n, m] {
        std::lock_guard<std::recursive_mutex> lock(*m);
        noop();
      };
    } break;

    case LockType::kShared: {
      auto m = std::make_shared<std::shared_mutex>();
      work = [n, m] {
        std::lock_guard<std::shared_mutex> lock(*m);
        noop();
      };
    } break;

    case LockType::kSpin: {
      auto m = std::make_shared<My::SpinMutex>();
      work = [n, m] {
        std::lock_guard<My::SpinMutex> lock(*m);
        noop();
      };
    } break;

    case LockType::kSpinRecursive: {
      auto m = std::make_shared<My::SpinMutex::Recursive>();
      work = [n, m] {
        std::lock_guard<My::SpinMutex::Recursive> lock(*m);
        noop();
      };
      break;
    }

    case LockType::kSpinShared: {
      auto m = std::make_shared<My::SpinMutex::Shared>();
      work = [n, m] {
        std::lock_guard<My::SpinMutex::Shared> lock(*m);
        noop();
      };
      break;
    }
  }

  std::cout << "lock " << lt << " for " << n << " * " << tn << " times."
            << std::endl;
  for (int _ = 0; _ < t; ++_) {
    auto timingStart = HRC::now();

    std::vector<std::thread> threads;
    for (std::uint32_t i = 0; i < tn; ++i) {
      threads.emplace_back([n, work] {
        for (std::uint32_t i = 0; i < n; ++i)
          work();
      });
    }
    for (auto&& t : threads)
      t.join();

    auto timingEnd = HRC::now();
    auto duration = timingEnd - timingStart;
    auto seconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() /
      1000.0;
    std::cout << _ << '\t';
    My::util::operator<<(std::cout, duration)
      << " (" << (double(n * tn) / seconds) << " /s, " << double(n) / seconds
      << " /s*tn)" << std::endl;
  }

  return 0;
}

} // namespace

// ========================================================================== //
// 文件打开速度 (TODO)
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
  { "create_threads", "", &create_threads },
  { "lock_mutex", "", &lock_mutex },
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
    ("all", "run all tests")                //
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

  if (vmap.count("all")) {
    for (auto&& i : kSubCmds) {
      std::vector<const char*> args{ i.mName };
      for (int i = opts.size(); i < argc; ++i)
        args.emplace_back(argv[i]);
      std::cout << "running: ";
      for (auto&& a : args)
        std::cout << ' ' << a;
      std::cout << std::endl;
      i.mFunc(args.size(), const_cast<char**>(args.data()));
      std::cout << std::endl;
    }
    return 0;
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
