#include "project.h"
#include <Lib/hpp>
#include <array>
#include <boost/program_options.hpp>
#include <iomanip>
#include <iostream>

using namespace std::string_literals;

namespace po = boost::program_options;

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

struct SubCmdFunc
{
  const char *mName, *mInfo;
  int (*mFunc)(int argc, char* argv[]);
};

const SubCmdFunc kSubCmdFuncs[] = {
  { "subcmd", "subcmd example", &subcmd },
  // TODO
};

int
main(int argc, char* argv[])
try {
  po::options_description od("Options");
  od.add_options()                          //
    ("version,v", "print version info")     //
    ("help,h", "print help info")           //
    ("...",                                 //
     po::value<std::vector<std::string>>(), //
     "sub arguments")                       //
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
  po::notify(vmap);

  if (vmap.count("version")) {
    std::cout << "Command-Line App"
                 "\n"
                 "\nBuilt: " __TIME__ " (" __DATE__ ")"
                 "\nProject: " UnnamedProject_VERSION "\n"
                 "\nCopyright (C) 2023 Yuhao Gu. All Rights Reserved."
              << std::endl;
    return 0;
  }

  if (vmap.count("help") || argc == 1) {
    std::cout << od
              << "\n"
                 "Sub Commands:\n";
    for (auto&& i : kSubCmdFuncs)
      std::cout << "  " << std::left << std::setw(12) << i.mName << i.mInfo
                << '\n';
    std::cout << "\n"
                 "[HINT: use '<subcmd> --help' to get help for sub commands.]\n"
              << std::endl;
    return 0;
  }

  if (opts.size() < argc) {
    std::string cmd = argv[opts.size()];
    for (auto&& i : kSubCmdFuncs) {
      if (cmd == i.mName)
        return i.mFunc(argc - opts.size(), argv + opts.size());
    }
    std::cout << "invalid sub command '" << cmd << "'." << std::endl;
    return 1;
  }
}

catch (Lib::Err& e) {
  std::cout << "\nERROR! " << e.what() << "\n" << e.info() << std::endl;
  return -3;
}

catch (std::exception& e) {
  std::cout << "\nERROR! " << e.what() << std::endl;
  return -2;
}

catch (...) {
  return -1;
}
