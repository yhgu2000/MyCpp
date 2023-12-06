#include "err.hpp"

namespace My {

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
  return *this;
}

std::string
Errno::info() const noexcept
{
#ifdef _MSC_VER
  std::string msg(128, '\0');
  strerror_s(msg.data(), msg.size(), mCode);
  msg.resize(msg.find('\0'));
  return msg;
#else
  std::string msg(128, '\0');
  auto str = strerror_r(mCode, msg.data(), msg.size());
  if (str != msg.data())
    msg = str;
  return msg;
#endif
}

std::string
BoostEC::info() const noexcept
{
  return mEc.message();
}

} // namespace err

} // namespace My
