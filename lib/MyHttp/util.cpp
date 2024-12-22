#include "util.hpp"

#include <My/err.hpp>
#include <My/util.hpp>

using namespace My::util;
using namespace My::log;

namespace MyHttp::util {

std::string
strsock(const Socket& sock)
{
  auto local = sock.local_endpoint();
  auto remote = sock.remote_endpoint();
  return local.address().to_string() + ':' + to_string(local.port()) + " -> " +
         remote.address().to_string() + ':' + to_string(remote.port());
}

thread_local std::minstd_rand gRandFast{ std::random_device()() };
thread_local std::mt19937_64 gRandSafe{ std::random_device()() };

bool
ThreadsExecutor::start()
{
  if (mThreads.front().joinable())
    return false;

  mIoCtx.get_executor().on_work_started();
  // 防止 mIoCtx.run() 在没有工作的情况下立即返回。

  for (auto& thread : mThreads)
    thread = std::thread([this] {
      while (true)
        try {
          mIoCtx.run();
          break;
        } catch (My::Err& e) {
          BOOST_LOG_SEV(mLogger, crit) << e.what() << '\n' << e.info();
        } catch (std::exception& e) {
          BOOST_LOG_SEV(mLogger, crit) << e.what();
        } catch (...) {
          BOOST_LOG_SEV(mLogger, fatal) << "UNKNOWN EXCEPTION";
        }
    });
  BOOST_LOG_SEV(mLogger, noti) << "started";
  return true;
}

bool
ThreadsExecutor::stop()
{
  if (!mThreads.front().joinable())
    return false;

  mIoCtx.get_executor().on_work_finished();
  mIoCtx.stop();
  for (auto&& thread : mThreads)
    thread.join();

  BOOST_LOG_SEV(mLogger, noti) << "stopped";
  return true;
}

bool
ThreadsExecutor::wait()
{
  if (!mThreads.front().joinable())
    return false;

  mIoCtx.get_executor().on_work_finished();
  for (auto&& thread : mThreads)
    thread.join();

  BOOST_LOG_SEV(mLogger, noti) << "waited";
  return true;
}

} // namespace MyHttp::util
