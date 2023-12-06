#include <boost/program_options.hpp>

namespace po = boost::program_options;

template<typename T>
static inline auto
pov(T& t)
{
  return boost::program_options::value(&t);
}

template<typename T>
static inline auto
povr(T& t)
{
  auto p = boost::program_options::value(&t);
  p->required();
  return p;
}

template<typename T>
static inline auto
povd(T& t)
{
  auto p = boost::program_options::value(&t);
  p->default_value(t);
  return p;
}

template<typename T, typename U>
static inline auto
povd(T& t, U u)
{
  auto p = boost::program_options::value(&t);
  p->default_value(t);
  p->implicit_value(u);
  return p;
}
