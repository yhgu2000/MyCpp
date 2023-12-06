#pragma once

#include "Standing.hpp"
#include <cassert>

namespace My {

/**
 * @brief 单例全局类的CRTP模板，可用于继承以实现系统模块间的代码解耦。
 */
template<typename T>
class Globally : public Standing
{
public:
  Globally()
  {
    assert(_g == nullptr);
    _g = static_cast<T*>(this);
  }

  ~Globally()
  {
    assert(_g == static_cast<T*>(this));
    _g = nullptr;
  }

  /**
   * @brief 获取全局单例对象的指针，若还未创建则返回空指针。
   */
  static constexpr T* g() { return static_cast<T*>(_g); }

protected:
  static T* _g;
};

template<typename T>
T* Globally<T>::_g = nullptr;

} // namespace My
