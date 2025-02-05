#include "ServerBuilder.hpp"
#include "HttpHelloWorld.hpp"
#include "HttpMatpowsum.hpp"
#include <My/err.hpp>
#include <fstream>

using namespace My::log;

namespace MyHttp {

void
ServerBuilder::register_server(const std::string& name, BuildServer buildServer)
{
  mBuilders.emplace(name, buildServer);
}

namespace {

std::unique_ptr<Server>
build_helloworld(Executor ex,
                 Executor acptEx,
                 std::string logName,
                 const bj::value& details)
{
  std::unique_ptr<HttpHelloWorld::Server> ret;
  if (!acptEx)
    ret = std::make_unique<HttpHelloWorld::Server>(ex, std::move(logName));
  else
    ret =
      std::make_unique<HttpHelloWorld::Server>(ex, acptEx, std::move(logName));
  ret->mConfig.jval_to(details);
  return ret;
}

std::unique_ptr<Server>
build_matpowsum(Executor ex,
                Executor acptEx,
                std::string logName,
                const bj::value& details)
{
  std::unique_ptr<HttpMatpowsum::Server> ret;
  if (!acptEx)
    ret = std::make_unique<HttpMatpowsum::Server>(ex, std::move(logName));
  else
    ret =
      std::make_unique<HttpMatpowsum::Server>(ex, acptEx, std::move(logName));
  ret->mConfig.jval_to(details);
  return ret;
}

} // namespace

void
ServerBuilder::register_builtins()
{
  register_server("HttpHelloWorld", &build_helloworld);
  register_server("HttpMatpowsum", &build_matpowsum);
}

ServerBuilder::Servers
ServerBuilder::build_jval(bj::value jval)
{
  Servers ret;
  for (const auto& [name, jcfg] : jval.as_object()) {
    ServerConfig cfg;
    cfg.jval_to(jcfg);

    auto it = mBuilders.find(cfg.mType);
    if (it == mBuilders.end()) {
      BOOST_LOG_SEV(mLogger, warn) << "unknown server type: " << cfg.mType;
      continue;
    }

    try {
      auto server = it->second(mExecutor, mAcptExecutor, name, cfg.mDetails);
      ret.emplace(name, std::make_pair(std::move(cfg), std::move(server)));
    } catch (My::Err& e) {
      BOOST_LOG_SEV(mLogger, warn) << "unable to build server " << name << '['
                                   << cfg.mType << ']' << ": " << e.info();
    } catch (std::exception& e) {
      BOOST_LOG_SEV(mLogger, crit) << "failed to build server " << name << '['
                                   << cfg.mType << ']' << ": " << e.what();
    }
  }
  return ret;
}

ServerBuilder::Servers
ServerBuilder::build_json(std::string_view json)
{
  bj::parse_options opt;
  opt.allow_comments = true;
  opt.allow_trailing_commas = true;
  return build_jval(bj::parse(json, {}, opt));
}

ServerBuilder::Servers
ServerBuilder::build_json_file(const std::string& path)
{
  std::ifstream ifs(path);
  if (!ifs)
    throw My::err::Str("failed to open file: " + path);
  bj::parse_options opt;
  opt.allow_comments = true;
  opt.allow_trailing_commas = true;
  return build_jval(bj::parse(ifs, {}, opt));
}

void
ServerBuilder::start_all(Servers& servers)
{
  for (auto& [name, pair] : servers) {
    auto& [cfg, server] = pair;
    if (!server)
      continue;
    Endpoint ep(ba::ip::make_address(cfg.mHost), cfg.mPort);
    server->start(ep, cfg.mBacklog);
  }
}

void
ServerBuilder::stop_all(Servers& servers)
{
  for (auto& [name, pair] : servers) {
    auto& [cfg, server] = pair;
    if (!server)
      continue;
    server->stop();
  }
}

bj::value
ServerBuilder::ServerConfig::to_jval() const noexcept
{
  bj::object ret;
  ret.emplace("Type", mType);
  ret.emplace("Host", mHost);
  ret.emplace("Port", mPort);
  ret.emplace("Backlog", mBacklog);
  ret.emplace("Details", mDetails);
  return ret;
}

void
ServerBuilder::ServerConfig::jval_to(bj::value jval) noexcept(false)
{
  auto jobj = jval.as_object();
  mType = jobj.at("Type").as_string();
  mHost = jobj.at("Host").as_string();
  mPort = jobj.at("Port").as_int64();
  mBacklog = jobj.at("Backlog").as_int64();
  mDetails = std::move(jobj.at("Details"));
}

} // namespace MyHttp
