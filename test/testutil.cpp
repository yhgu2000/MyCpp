#include "testutil.hpp"
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

void
noopt_impl(void* a)
{
  assert(!a);
}

static int gLogLevel;

void
init_loglevel(int logLevel)
{
  gLogLevel = logLevel;
  boost::log::add_common_attributes();
  boost::log::add_console_log(std::clog,
                              boost::log::keywords::format = &My::log::format);
  boost::log::core::get()->set_filter(
    [](const boost::log::attribute_value_set& attrs) {
      return attrs["Severity"].extract<My::log::Level>() >= gLogLevel;
    });
}

void
reset_loglevel(int logLevel)
{
  gLogLevel = logLevel;
}

namespace randgen {

thread_local std::default_random_engine gRand{ std::random_device()() };

std::vector<double>
split(size_t n)
{
  std::vector<double> ret(n);
  std::uniform_real_distribution<> dis(0, 1.0 / n);
  for (std::size_t i = 0; i < n; ++i)
    ret[i] = double(i) / n + dis(gRand);
  return ret;
}

} // namespace randgen
