#include "CFile64.hpp"

namespace My {

std::string
CFile64::load_s(const char* path) noexcept(false)
{
  CFile64 file(path, "rb");
  Closer closer(file);
  return file.rest_s();
}

Bytes
CFile64::load_b(const char* path) noexcept(false)
{
  CFile64 file(path, "rb");
  Closer closer(file);
  return file.rest_b();
}

std::string
CFile64::rest_s() const noexcept(false)
{
  std::int64_t size;
  {
    Seeker seeker(*this, 0, SEEK_END);
    size = tell() - seeker.mPos;
  }
  std::string ret(size, {});
  read(ret.data(), size, 1);
  return ret;
}

Bytes
CFile64::rest_b() const noexcept(false)
{
  std::int64_t size;
  {
    Seeker seeker(*this, 0, SEEK_END);
    size = tell() - seeker.mPos;
  }
  Bytes ret(size, {});
  read(ret.data(), size, 1);
  return ret;
}

} // namespace My
