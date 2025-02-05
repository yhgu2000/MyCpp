#pragma once

#include "cpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <memory>
#include <set>

namespace boost::json {
class value;
};

namespace My {
class Timing;
} // namespace My

/**
 * @brief 计时宏，宏参数可以是表达式，也可以是语句块，返回纳秒级计时。
 */
#define MY_TIMING(code)                                                        \
  [&]() {                                                                      \
    auto __timing_begin__ = std::chrono::high_resolution_clock::now();         \
    code;                                                                      \
    auto __timing_end__ = std::chrono::high_resolution_clock::now();           \
    return std::chrono::duration_cast<std::chrono::nanoseconds>(               \
      __timing_end__ - __timing_begin__);                                      \
  }()

/**
 * @brief n 次计时宏，宏参数可以是表达式，也可以是语句块，返回纳秒级计时。
 */
#define MY_NIMING(n, code)                                                     \
  [&]() {                                                                      \
    auto __niming_begin__ = std::chrono::high_resolution_clock::now();         \
    for (std::size_t __niming_n__ = 0; __niming_n__ < n; ++__niming_n__)       \
      code;                                                                    \
    auto __niming_end__ = std::chrono::high_resolution_clock::now();           \
    return std::chrono::duration_cast<std::chrono::nanoseconds>(               \
      __niming_end__ - __niming_begin__);                                      \
  }()

/**
 * @brief 标准输出流打印函数，以缩进文本的方式打印输出。
 */
std::ostream&
operator<<(std::ostream& out, const My::Timing& prof);

namespace My {

namespace bj = boost::json;

/**
 * @brief 一个好用的计时类，线程安全，支持导入导出 JSON，并且可以通过重载虚函数
 * 的方式在子类中监视计时动作。
 */
class Timing
{
public:
  using Clock = std::chrono::high_resolution_clock;

  /**
   * @brief 用于给记录提供额外信息的接口类。
   */
  struct alignas(alignof(std::size_t)) Info
  {
    virtual ~Info() = default;

    /**
     * @brief 获取附加信息。
     *
     * 如果计时序列会被并发访问，那么该方法必须是线程安全的。
     */
    virtual std::string info() noexcept = 0;
  };

  /**
   * @brief 保存一个字符串的简单信息类。
   */
  struct StrInfo
    : public Info
    , public std::string
  {
    using std::string::string;
    std::string info() noexcept override { return *this; }
  };

  class Entry;
  class Iterator;
  class Scope;

public:
  /**
   * @brief 从 JSON 导入。
   *
   * @param json JSON 对象。
   * @param tags 用于保存标签字符串所有权的集合容器。
   *
   * @note \p tags 不能为 unordered，以免发生重哈希时返回对象中的指针失效！
   */
  static Timing from_json(const bj::value& json,
                          std::set<std::string>& tags) noexcept(false);

public:
  virtual ~Timing() = default;

  /**
   * @brief 构造函数，构造时刻为初始计时点。
   */
  Timing();

  /**
   * @brief 该构造函数是浅拷贝，拷贝后的对象与原对象共享计时序列。
   */
  Timing(const Timing&) = default;

  /**
   * @brief 该赋值操作是浅拷贝，拷贝后的对象与原对象共享计时序列。
   */
  Timing& operator=(const Timing&) = default;

  /**
   * @brief 记录一次计时。
   *
   * 该方法是线程安全的，多个线程调用同一个 Timing 对象的该方法、或者多个线程
   * 调用指向同一计时序列的该方法都不会发生并发竞争。
   *
   * @param tag 计时标签。
   * @param info 附加信息，可为空。
   * @param owned 是否托管 info，若 info 为空则此参数必须为 false。
   *
   * @return 本次计时构造的记录条目，是引用，当心指针悬挂。
   */
  Entry& operator()(const char* tag,
                    Info* info = nullptr,
                    bool owned = false) noexcept;

public:
  /**
   * @brief 获取 Timing 的创建的时刻。
   */
  Clock::time_point initial() const noexcept;

  /**
   * @brief 导出到 JSON 值对象。
   */
  bj::value to_jval() const noexcept(false);

public:
  ///@name 迭代器。
  ///@{
  Iterator begin() const noexcept;
  Iterator end() const noexcept;
  ///@}

protected:
  /**
   * @brief 在子类中重载这个方法以监视计时。
   *
   * 默认实现对标签进行筛选并打印到 cout。筛选方式是将 TIMING_MONITOR_FILTER
   * 解释为 ECMAScript 正则表达式，如果该环境变量未设置，则不打印任何信息。
   *
   * @param ent 本次计时构造的记录条目。
   */
  virtual void monitor(Entry& entry) noexcept;

private:
  std::shared_ptr<Entry> mHead;

private:
  friend std::ostream& ::operator<<(std::ostream & out, const Timing & prof);
};

/**
 * @brief 记录条目。
 *
 * TODO 或许定制内存分配器可以更快？
 */
class Timing::Entry
{
  friend class Timing;

public:
  const char* mTag;        ///< 计时标签
  Clock::time_point mTime; ///< 计时点

public:
  Entry(const Entry&) = delete;
  Entry(Entry&&) = delete;
  Entry& operator=(const Entry&) = delete;
  Entry& operator=(Entry&&) = delete;

  ~Entry() noexcept;

public:
  /**
   * @brief 获取记录上的附加消息，如果没有则返回空。
   */
  std::string info()
  {
    if (mInfo)
      return mInfo->info();
    return {};
  }

  /**
   * @brief 获取消息对象
   *
   * @return 可能为空。
   */
  Info* get_info()
  {
    return reinterpret_cast<Info*>(reinterpret_cast<std::size_t>(mInfo) &
                                   ~std::size_t(1));
  }

  /**
   * @brief 设置消息对象。
   *
   * 这个方法不会检查是否持有原消息对象的所有权，调用方应当在调用前检查并释放。
   *
   * @param info 消息对象。
   * @param owned 是否转移所有权，如果 info 为空，则必须为 false。
   */
  void set_info(Info* info, bool owned)
  {
    assert(info || !owned); // info为空时，owned必须为false
    mInfo = info;
    reinterpret_cast<std::size_t&>(mInfo) |= int(owned);
  }

  /**
   * @brief 检查是否持有消息对象的所有权。
   */
  bool info_owned()
  {
    return reinterpret_cast<std::size_t>(mInfo) & std::size_t(1);
  }

private:
  Info* mInfo; ///< 附加信息，最低位为所有权标记
  std::atomic<Entry*> mNext;

private:
  Entry() = default;

  Entry(Clock::time_point time,
        const char* tag,
        Info* info,
        bool owned,
        Entry* next)
    : mInfo(info)
    , mTag(tag)
    , mTime(time)
    , mNext(next)
  {
    reinterpret_cast<std::size_t&>(mInfo) |= int(owned);
  }
};

class Timing::Iterator
{
public:
  Iterator(Entry* entry) noexcept
    : mEntry(entry)
  {
  }

public:
  Iterator& operator++() noexcept
  {
    mEntry = mEntry->mNext.load();
    return *this;
  }

  Iterator operator++(int) noexcept
  {
    Iterator tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==(const Iterator& other) const noexcept
  {
    return mEntry == other.mEntry;
  }

  bool operator!=(const Iterator& other) const noexcept
  {
    return mEntry != other.mEntry;
  }

  Entry& operator*() const noexcept { return *mEntry; }

  Entry* operator->() const noexcept { return mEntry; }

private:
  Entry* mEntry;
};

/**
 * @brief 作用域计时类，在构造时进行一次计时，在析构时再自动进行一次计时
 */
class Timing::Scope
{
public:
  struct EnterInfo : public Info
  {
    std::string info() noexcept override;
  };

  struct LeaveInfo : public Info
  {
    std::string info() noexcept override;
  };

public:
  static EnterInfo gEnterInfo; ///< 作用域进入时的附加信息
  static LeaveInfo gLeaveInfo; ///< 作用域离开时的附加信息

public:
  Scope(Timing& self, const char* tag)
    : _(self)
    , mTag(tag)
  {
    _(mTag, &gEnterInfo, false);
  }

  Scope(const Scope&) = delete;
  Scope(Scope&&) = delete;
  Scope& operator=(const Scope&) = delete;
  Scope& operator=(Scope&&) = delete;

  ~Scope() { _(mTag, &gLeaveInfo, false); }

private:
  Timing& _;
  const char* mTag;
};

} // namespace My

namespace My {

inline Timing::Timing()
  : mHead(new Entry(Clock::now(), nullptr, nullptr, false, nullptr))
{
}

inline Timing::Clock::time_point
Timing::initial() const noexcept
{
  return mHead->mTime;
}

inline Timing::Iterator
Timing::begin() const noexcept
{
  return Iterator(mHead->mNext.load(std::memory_order_relaxed));
}

inline Timing::Iterator
Timing::end() const noexcept
{
  return Iterator(nullptr);
}

} // namespace My
