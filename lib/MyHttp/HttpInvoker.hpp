#pragma once

#include "util.hpp"

#include <My/log.hpp>

namespace MyHttp {

using namespace util;

class HttpInvoker : public std::enable_shared_from_this<HttpInvoker>
{
public:
  void start();

  void stop();

protected:
  My::log::Logger mLogger;
  Request mRequest;
  Response mResponse;

  HttpInvoker(Socket&& sock,
              Config& config,
              const char* logName = "HttpInvoker")
    : mLogger(logName, this)
    , mConfig(config)
    , mStream(std::move(sock))
    , mBuffer(config.mBufferLimit)
  {
  }
};

} // namespace MyHttp
