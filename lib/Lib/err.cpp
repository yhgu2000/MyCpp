#include "err.hpp"

namespace Lib {

using namespace std::string_literals;

const char*
Err::what() const noexcept
{
  return typeid(*this).name();
}

namespace err {

std::string
Lit::info() const noexcept
{
  return what();
}

std::string
Str::info() const noexcept
{
  return mWhat;
}

std::string
Errno::info() const noexcept
{
#ifdef _MSC_VER
  std::string msg(64, '\0');
  strerror_s(msg.data(), msg.size(), mCode);
  msg.resize(msg.find('\0'));
  return msg;
#else
  return // TODO
#endif
}

} // namespace err

} // namespace Lib
