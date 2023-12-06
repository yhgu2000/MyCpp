#pragma once

#include "cpp"

#include <boost/system/error_code.hpp>
#include <cassert>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

namespace My {

namespace err {
class Lit;
class Str;
} // namespace err

class Err : public std::exception
{
public:
  Err() = default;

public:
#ifndef NDEBUG
  /**
   * @brief 用于方便调试，将错误信息打印到标准输出。
   */
  void cout() const { std::cout << info() << std::endl; }
#endif

  /**
   * @brief 构造并返回人类可读的错误信息字符串。
   */
  virtual std::string info() const noexcept = 0;

public:
  ///@name exception interface
  ///@{
  const char* what() const noexcept override;
  ///@}

#ifdef _MSC_VER
private:
  friend class err::Lit;
  friend class err::Str;

  Err(const char* const msg)
    : std::exception(msg)
  {
  }
#endif
};

namespace err {

class Lit : public Err
{
public:
#ifdef _MSC_VER
  Lit(const char* const what)
    : Err(what)
  {
    assert(what != nullptr);
  }

  using std::exception::what;

#else
  const char* const mWhat;

  Lit(const char* const what)
    : mWhat(what)
  {
    assert(what != nullptr);
  }

public:
  ///@name exception interface
  ///@{
  const char* what() const noexcept override { return mWhat; }
  ///@}
#endif

public:
  ///@name Err interface
  ///@{
  std::string info() const noexcept override;
  ///@}
};

class Str
  : public Err
  , public std::string
{
public:
  using std::string::string;

  Str(std::string what)
    : std::string(std::move(what))
  {
  }

public:
  ///@name Err interface
  ///@{
  std::string info() const noexcept override;
  ///@}
};

class Errno : public Err
{
public:
  const int mCode;

public:
  Errno(int code)
    : mCode(code)
  {
  }

public:
  ///@name Err interface
  ///@{
  std::string info() const noexcept override;
  ///@}
};

class BoostEC : public Err
{
public:
  const boost::system::error_code mEc;

public:
  BoostEC(boost::system::error_code ec)
    : mEc(std::move(ec))
  {
  }

public:
  ///@name Err interface
  ///@{
  std::string info() const noexcept override;
  ///@}
};

} // namespace err

} // namespace My
