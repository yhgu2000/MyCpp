#pragma once

#include "HttpHandler.hpp"
#include "Server.hpp"

namespace MyHttp::servers {

class HelloWorld : public Server
{
public:
  /// HTTP 处理器的配置，运行中不可更改。
  HttpHandler::Config mConfig;

  HelloWorld(Executor ex)
    : Server(std::move(ex), "servers::HelloWorld")
  {
  }

private:
  void come(Socket&& sock) override;
};

class Matpowsum : public Server
{
public:
  /// HTTP 处理器的配置，运行中不可更改。
  HttpHandler::Config mConfig;

  Matpowsum(Executor ex)
    : Server(std::move(ex), "servers::Matpowsum")
  {
  }

private:
  void come(Socket&& sock) override;
};

} // namespace MyHttp::servers
