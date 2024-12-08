// TODO

#pragma once

#include <atomic>
#include <cassert>
#include <memory>

namespace My {

namespace _MultiPooled {

class Node : std::enable_shared_from_this<Node>
{
public:
  virtual ~Node() = default;

private:
  std::shared_ptr<Node> mNext;
  Node* mPrev; // 最低位用作自旋锁，次低位用作桩指示
};

class Stub : Node
{
private:
  Stub* mNextStub;
  Stub* mPrevStub;
};

} // namespace _MultiPooled

/**
 * @brief 改进的多线程资源池：
 * 1. 允许多个资源插入和申请点，避免单个桩成为瓶颈；
 * 2. 资源池本身在多线程间共享，与最后一个桩一同销毁；
 * 3. 弱化的资源申请语义：资源申请失败时资源池可能不为空。
 *
 * @tparam T 资源类型。
 */
template<typename T>
class MultiPooled : public _MultiPooled::Node
{
  class Pool;
};

template<typename T>
class MultiPooled<T>::Pool
{
private:
  std::shared_ptr<_MultiPooled::Stub> mStub;
};

} // namespace My
