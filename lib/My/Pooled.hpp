#pragma once

#include <atomic>
#include <cassert>

namespace My {

/**
 * @brief 池化混入类，用在并发编程中实现无锁的资源池。
 *
 * 示例：
 *
 * ```cpp
 * struct Resource : public Pooled<Resource> {};
 * Resource::Pool pool;
 * auto r = new Resource();
 * pool.give(r1);
 * r = pool.take();
 * ```
 *
 * @tparam T 资源类型。
 */
template<typename T>
class Pooled
{
public:
  using Pool = Pooled<T>;

  Pooled()
    : mNextPooled(nullptr)
    , mPrevPooled(nullptr)
  {
  }

  /**
   * @brief 归还当前资源，不可重复归还。
   *
   * @param t 任何一个在池内的资源，将当前资源追加到它的后面。
   */
  void put_back(Pool& t)
  {
    // TODO
  }

  /**
   * @brief 取出当前资源。
   */
  void take_out()
  {
    // TODO
  }

  /**
   * @brief 从池中任取一个资源。
   *
   * @return T* 下一个资源，如果池空则返回 nullptr。
   */
  T* take_one()
  {
    // TODO
  }

private:
  std::atomic<T*> mNextPooled;
  std::atomic<T*> mPrevPooled;
};

} // namespace My
