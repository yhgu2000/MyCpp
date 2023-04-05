#pragma once

#include "cpp"
#include <cassert>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

namespace Lib {

namespace err {
class Lit;
class Str;
}

class Err : public std::exception
{
public:
  Err() {}

public:
  /**
   * @brief 用于调试，将错误信息打印到标准输出。
   */
  void cout()
  {
#ifndef NDEBUG
    std::cout << info() << std::endl;
#endif
  }

  /**
   * @brief 构造并返回人类可读的错误信息字符串。
   */
  virtual std::string info() const noexcept { return what(); }

public:
  ///@name exception interface
  ///@{
  virtual const char* what() const noexcept override
  {
    return "class ::Lib::Err;";
  }
  ///@}

#ifdef _MSC_VER
private:
  friend class err::Lit;
  friend class err::Str;

  Err(const char* msg)
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

  Lit(const char* what)
    : Err(what)
  {
    assert(what != nullptr);
  }

  using std::exception::what;

#else
  const char* _what;

  Lit(const char* what)
    : _what(what)
  {
    assert(what != nullptr);
  }

public:
  ///@name exception interface
  ///@{
  virtual const char* what() const noexcept override { return _what; }
  ///@}
#endif

public:
  ///@name Err interface
  ///@{
  virtual std::string info() const noexcept override { return what(); }
  ///@}
};

class Str : public Err
{
public:
  std::string _what;

public:
#ifdef _MSC_VER
  Str(std::string what)
    : Err(what.c_str())
    , _what(std::move(what))
  {
  }

#else
  Str(std::string what)
    : _what(std::move(what))
  {
  }

public:
  ///@name exception interface
  ///@{
  virtual const char* what() const noexcept override { return _what.c_str(); }
  ///@}
#endif

public:
  ///@name Err interface
  ///@{
  virtual std::string info() const noexcept override { return _what; }
  ///@}
};

class Errno : public Err
{
public:
  int _code;

public:
  Errno(int code)
    : _code(code)
  {
  }

public:
  ///@name exception interface
  ///@{
  virtual const char* what() const noexcept override
  {
    return std::strerror(_code);
  }
  ///@}

public:
  ///@name Err interface
  ///@{
  virtual std::string info() const noexcept override
  {
    return std::strerror(_code);
  }
  ///@}
};

}

}
