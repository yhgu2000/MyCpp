#include "util.hpp"

namespace genrand {

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

}
