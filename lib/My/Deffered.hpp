#pragma once

namespace My {

/**
 * @brief 用于推迟变量构造的工具类。
 */
template<typename T>
class Deffered final
{
public:
  class Guard;

  operator T&() { return operator*(); }
  T& operator*() { return *operator->(); }
  T* operator->() { return reinterpret_cast<T*>(_); }

  template<typename... Args>
  void ctor(Args... args)
  {
    new (_) T(args...);
  }

  void dtor() { operator*().~T(); }

private:
  char _[sizeof(T)];
};

template<typename T>
class Deffered<T>::Guard final
{
public:
  Deffered<T>& _;

  template<typename... Args>
  Guard(Deffered<T>& _, Args... args)
    : _(_)
  {
    _.ctor(args...);
  }

  ~Guard() { _.dtor(); }
};

} // namespace Common
