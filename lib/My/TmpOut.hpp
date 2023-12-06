#pragma once

#include <sstream>
#include <iostream>

namespace My {

/**
 * @brief 在内存中缓冲输出，直到全部输出完毕后再提交给输出流。
 */
struct TmpOut : public std::stringstream
{
  TmpOut(std::ostream& out, std::ios_base::openmode mode = std::ios::out)
    : std::stringstream(mode)
    , mOut(&out)
  {
  }

  ~TmpOut() override
  {
    if (mOut)
      *mOut << str() << std::flush;
  }

  TmpOut(TmpOut&& other) noexcept
    : std::stringstream(std::move(other))
    , mOut(other.mOut)
  {
    other.mOut = nullptr;
  }

  TmpOut& operator=(TmpOut&& other) noexcept
  {
    if (this != &other) {
      std::stringstream::operator=(std::move(other));
      mOut = other.mOut;
      other.mOut = nullptr;
    }
    return *this;
  }

private:
  std::ostream* mOut{ nullptr };
};

inline TmpOut
tout()
{
  return { std::cout };
}

inline TmpOut
terr()
{
  return { std::cerr };
}

} // namespace My
