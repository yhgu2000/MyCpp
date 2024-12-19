#include "Pooled.hpp"

namespace My::_Pooled {

Iterator&
Iterator::operator++() noexcept
{
  auto prev = std::move(mNode);
  mNode = prev->mNext;
  SpinBit::unlock(prev->mPrev);
  if (mNode)
    SpinBit::lock(mNode->mPrev);
  return *this;
}

std::shared_ptr<Node>
Stub::take(Node& after) noexcept
{
  auto prev = after.shared_from_this();
  SpinBit::lock(prev->mPrev);

  auto here = std::move(prev->mNext);
  if (!here) {
    SpinBit::unlock(prev->mPrev);
    return nullptr;
  }
  SpinBit::lock(here->mPrev);

  auto next = std::move(here->mNext);
  if (!next) {
    // std::move 已经将 prev->mNext 置空
    SpinBit::unlock(prev->mPrev); // 尽可能早地释放锁
    here->mPrev.store(0, std::memory_order_release);
    // 上面这一步同时释放了 here 的锁
    return here;
  }
  SpinBit::lock(next->mPrev);

  next->mPrev.store(reinterpret_cast<std::uintptr_t>(prev.get()),
                    std::memory_order_release);
  // 上面这一步同时释放了 next 的锁（&prev 的最低位一定为 0）
  prev->mNext = next;
  SpinBit::unlock(prev->mPrev);

  // std::move 已经将 here->mNext 置空
  here->mPrev.store(0, std::memory_order_release);
  // 上面这一步同时释放了 here 的锁
  return here;
}

std::shared_ptr<Node>
Stub::take_if(Node& after, const std::type_info& type) noexcept
{
  auto prev = after.shared_from_this();
  SpinBit::lock(prev->mPrev);

  while (true) {
    if (!prev->mNext) {
      SpinBit::unlock(prev->mPrev);
      return nullptr;
    }
    auto here = std::move(prev->mNext);
    SpinBit::lock(here->mPrev);

    if (typeid(*here) != type) {
      SpinBit::unlock(prev->mPrev);
      prev = std::move(here);
      continue;
    }

    auto next = std::move(here->mNext);
    if (!next) {
      // std::move 已经将 prev->mNext 置空
      SpinBit::unlock(prev->mPrev); // 尽可能早地释放锁
      here->mPrev.store(0, std::memory_order_release);
      // 上面这一步同时释放了 here 的锁
      return here;
    }
    SpinBit::lock(next->mPrev);

    next->mPrev.store(reinterpret_cast<std::uintptr_t>(prev.get()),
                      std::memory_order_release);
    // 上面这一步同时释放了 next 的锁（&prev 的最低位一定为 0）
    prev->mNext = next;
    SpinBit::unlock(prev->mPrev);

    // std::move 已经将 here->mNext 置空
    here->mPrev.store(0, std::memory_order_release);
    // 上面这一步同时释放了 here 的锁
    return here;
  }
}

void
Stub::give(Node& after, std::shared_ptr<Node> here) noexcept
{
  assert(&after && here);
  auto prev = &after;

  // 由于 here 不在链上，所以我们可以先锁定它
  SpinBit::lock(here->mPrev);
  // 在锁定之后再断言，尽量避免错误编程的影响
  assert(here->mNext == nullptr);

  SpinBit::lock(prev->mPrev);
  auto next = std::move(prev->mNext);
  if (!next) {
    here->mPrev.store(reinterpret_cast<std::uintptr_t>(prev),
                      std::memory_order_release);
    // 上面这一步同时释放了 here 的锁（&prev 的最低位一定为 0）
    prev->mNext = std::move(here);
    SpinBit::unlock(prev->mPrev);
    return;
  }
  SpinBit::lock(next->mPrev);

  next->mPrev.store(reinterpret_cast<std::uintptr_t>(here.get()),
                    std::memory_order_release);
  // 上面这一步同时释放了 next 的锁（&here 的最低位一定为 0）
  here->mNext = std::move(next);
  here->mPrev.store(reinterpret_cast<std::uintptr_t>(prev),
                    std::memory_order_release);
  // 上面这一步同时释放了 here 的锁（&prev 的最低位一定为 0）

  prev->mNext = std::move(here);
  SpinBit::unlock(prev->mPrev);
}

void
Stub::drop(Node& node) noexcept
{
  assert(&node);
  auto here = node.shared_from_this();
  while (true) {
    SpinBit::lock(here->mPrev);
    auto masked = SpinBit::masked(here->mPrev);
    if (!masked) {
      SpinBit::unlock(here->mPrev);
      return;
    }
    auto prev = reinterpret_cast<Node*>(masked)->shared_from_this();
    assert(prev);

    // 先释放 hereBit，等 prevBit 锁定后再重新锁定，避免死锁
    SpinBit::unlock(here->mPrev);
    SpinBit::lock(prev->mPrev);
    if (prev->mNext != here) {
      SpinBit::unlock(prev->mPrev);
      continue; // 重试
    }
    SpinBit::lock(here->mPrev); // 重新按顺序锁定

    auto next = std::move(here->mNext);
    if (!next) {
      prev->mNext.reset();
      SpinBit::unlock(prev->mPrev); // 尽可能早地释放锁
      here->mPrev.store(0, std::memory_order_release);
      // 上面这一步同时释放了 here 的锁
      return;
    }

    SpinBit::lock(next->mPrev);
    next->mPrev.store(reinterpret_cast<std::uintptr_t>(prev.get()),
                      std::memory_order_release);
    // 上面这一步同时释放了 next 的锁（&prev 的最低位一定为 0）
    prev->mNext = std::move(next);
    SpinBit::unlock(prev->mPrev); // 尽可能早地释放锁

    here->mPrev.store(0, std::memory_order_release);
    // 上面这一步同时释放了 here 的锁
    break;
  }
}

void
Stub::clear(Node& after) noexcept
{
  auto prev = after.shared_from_this();
  SpinBit::lock(prev->mPrev);

  auto here = std::move(prev->mNext);
  SpinBit::unlock(prev->mPrev);
  while (here) {
    SpinBit::lock(here->mPrev);
    auto next = std::move(here->mNext);
    here->mPrev.store(0, std::memory_order_release);
    // 上面这一步同时释放了 here 的锁
    here = std::move(next);
    // here 指向的对象会直到在引用计数为 0 才被销毁
  }
}

std::size_t
Stub::count(Node& node) noexcept
{
  std::size_t cnt = 0;
  for (auto it = begin(node), end = Stub::end(); it != end; ++it)
    ++cnt;
  return cnt;
}

} // namespace My::_Pooled

/**
 * 一些设计思考：
 *
 * 1. shared_from_this 必要吗？
 *
 *    似乎是不必要的，因为：
 *    - 资源上的锁总保证其只会被单个线程占用，不存在多线程间析构的问题
 *    - 所以，资源的所有权要么是资源池，要么是单个线程，不存在多线程间共享的问题
 *
 *    如果不用智能指针呢？当线程移交资源到池时，谁持有资源的所有权？所以必须要用。
 *
 *    如果使用 unique_ptr 呢？那么如果资源要在多线程间共享，它岂不是会同时拥有
 *    shared_ptr 和 unique_ptr 两种所有权？这是肯定不对的。
 *    - 除非：unique_ptr<shared_ptr<T>>，这样就可以保证资源只有一个所有权
 *    - 得这么考虑：一般使用这个类的资源都不是线程间共享（所有权）的，所以不会有
 *      这个问题
 *
 *    主要是在 drop 操作中，由于要按序锁定，在从 here 找到 prev 时，需要放弃
 *    here 的锁， 而从锁被放弃到找到 prev 之间，prev 就可能被释放了，因此必须
 *    要有一种办法来保证 prev 到下次锁定前不会被释放，这就是 shared_from_this
 *    的作用。
 *
 *    结论：有必要，就是为了 drop 操作服务的
 *
 * 2. 要不要有哨兵？
 *
 *    哨兵的问题是：
 *    - 类型不安全，shared_ptr/unique_ptr 在转换时可能会发生切片
 *
 *    没有哨兵的问题是：
 *    - 当池中仅剩的一个资源被丢弃时，怎么更新资源池的头指针？
 *
 *    结论：有必要，否则无法实现资源池的头指针更新
 *
 *    那要不要有尾哨兵？
 *    - 有尾哨兵的问题是：尾哨兵的存在会对遍历带来问题，因为遍历器要持有节点上
 *      的锁 —— 那么 end() 返回的尾哨兵要不要持有锁呢？如果持有锁，那么多线程
 *      遍历就是不可能的：尾哨兵上会互斥。
 *
 *    结论：不能有尾哨兵，因为遍历器不能持有尾哨兵上的锁
 *
 * 3. 要不要用 virtual 析构函数？
 *
 *    设想这样一种情况：一个线程正在执行 drop，它通过 prev 给前一个结点刚刚
 *    创建了一个 shared_ptr<Node> 的指针，这个结点就被另一个线程 drop 掉了，
 *    但因为这个线程持有了 shared_ptr，所以这个结点没有在另一个线程 drop
 *    时被销毁，而是在 shared_ptr 被销毁时被销毁，这时结点会被切片（以 Node
 *    析构，而非 T），这问题就大了。
 *
 *    此外，virtual 析构函数可以避免模板导致的代码膨胀，因为大部分对象头处理
 *    的代码都可以放到内部命名空间中。
 *
 *    结论：要，因为有哨兵后可能会发生切片。
 *
 * 4. 要不要区分 next 和 prev 锁？
 *
 *    没必要，因为争用主要发生在申请资源时，仅仅隔离申请和释放不会减轻争用。
 *
 * 5. 因为有了析构函数，可以考虑搞成一个 RAII 的资源池，每个结点可以是不同类
 *    型的资源。需要增加额外的方法：取出下一个指定类型的资源。当然，可以再用
 *    模板提供一个类型安全的类，它使用静态类型转换避免运行时类型检查。
 */
