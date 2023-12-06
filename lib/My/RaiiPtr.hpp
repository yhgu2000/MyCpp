#pragma once

#include <utility>

namespace My {

/**
 * @brief RAII模式的可移动资源指针模板类。
 */
template<typename T, void (*free)(T*)>
class RaiiPtr
{
public:
  RaiiPtr(T* p = nullptr) noexcept
    : _(p)
  {
  }

  RaiiPtr(const RaiiPtr&) = delete;

  RaiiPtr(RaiiPtr&& other) noexcept
    : RaiiPtr()
  {
    swap(other);
  }

  ~RaiiPtr() noexcept
  {
    if (_ != nullptr)
      free(_);
  }

  operator bool() const { return _ != nullptr; }

  operator T*() const { return _; }

  RaiiPtr& operator=(const RaiiPtr&) = delete;

  RaiiPtr& operator=(RaiiPtr&& other) noexcept
  {
    swap(*this, other);
    return *this;
  }

  T* operator->() const { return _; }

  T& operator*() const { return *operator->(); }

  T* operator+() const { return operator->(); }

  T* operator-()
  {
    auto p = _;
    _ = nullptr;
    return p;
  }

  friend void swap(RaiiPtr& lhs, RaiiPtr& rhs) noexcept
  {
    using std::swap;
    swap(lhs._, rhs._);
  }

private:
  T* _;
};

} // namespace My
