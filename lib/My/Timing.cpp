#include "Timing.hpp"
#include "util.hpp"

#include <boost/json.hpp>
#include <iostream>
#include <regex>
#include <vector>

using namespace My::util;
namespace sc = std::chrono;

std::ostream&
operator<<(std::ostream& out, const My::Timing& prof)
{
  using namespace My;

  std::vector<Timing::Entry*> stack;
  auto last = prof.initial();
  for (auto&& i : prof) {
    for (auto i = stack.size(); i > 1; --i)
      out << '\t';

    auto info = i.get_info();
    if (info == &Timing::Scope::gLeaveInfo && !stack.empty() &&
        stack.back()->mTag == i.mTag) {
      out << i.mTag << " [" << (i.mTime - stack.back()->mTime) << ']';
      stack.pop_back();
    }

    else {
      if (!stack.empty())
        out << '\t';
      out << i.mTag << " [" << (i.mTime - last) << ']';
      if (info) {
        if (info == &Timing::Scope::gEnterInfo)
          stack.push_back(&i);
        else
          out << " : " << info->info();
      }
      out << '\n';
    }

    last = i.mTime;
  }

  return out;
}

namespace My {

Timing
Timing::from_json(const bj::value& json,
                  std::set<std::string>& tags) noexcept(false)
{
  const auto& arr = json.as_array();

  Timing prof;
  for (auto&& i : arr) {
    const auto& ent = i.as_array();

    const auto* tag =
      tags.emplace(ent.at(0).as_string().c_str()).first->c_str();

    auto time = prof.mHead->mTime +
                sc::duration_cast<Clock::duration>(
                  sc::duration<double, std::nano>(ent.at(1).as_double()));

    Info* info;
    if (ent.size() <= 2)
      info = nullptr;
    else
      info = new StrInfo(ent.at(2).as_string().c_str());

    prof.mHead->mNext.store(
      new Entry(time,
                tag,
                info,
                bool(info),
                prof.mHead->mNext.load(std::memory_order_relaxed)),
      std::memory_order_relaxed);
  }

  return prof;
}

Timing::Entry&
Timing::operator()(const char* tag, Info* info, bool owned) noexcept
{
  assert(info || !owned); // info为空时，owned必须为false

  auto* entry = new Entry(Clock::now(), tag, info, owned, nullptr);
  auto* next = mHead->mNext.load(std::memory_order_relaxed);
  do {
    entry->mNext.store(next, std::memory_order_relaxed);
  } while (!mHead->mNext.compare_exchange_weak(
    next, entry, std::memory_order_relaxed));

  monitor(*entry);
  return *entry;
}

bj::value
Timing::to_jval() const noexcept(false)
{
  bj::array arr;

  for (auto&& i : *this) {
    bj::array ent;
    ent.emplace_back(i.mTag);
    sc::duration<double, std::nano> dura(i.mTime - mHead->mTime);
    ent.emplace_back(dura.count());
    if (i.mInfo)
      ent.emplace_back(i.mInfo->info());
    arr.emplace_back(std::move(ent));
  }

  return arr;
}

void
Timing::monitor(Entry& entry) noexcept
{
  static const auto kReportFilter = []() -> std::unique_ptr<std::regex> {
    char re[32];
    size_t len;
    getenv_s(&len, re, sizeof(re), "TIMING_MONITOR_FILTER");
    if (len == 0)
      return nullptr;
    return std::make_unique<std::regex>(re);
  }();

  if (kReportFilter == nullptr || !std::regex_match(entry.mTag, *kReportFilter))
    return;

  std::cout << (entry.mTime - initial()) << " " << entry.mTag;
  if (entry.mInfo)
    std::cout << " " << entry.mInfo->info();
  std::cout << '\n';
}

Timing::Scope::EnterInfo Timing::Scope::gEnterInfo;
Timing::Scope::LeaveInfo Timing::Scope::gLeaveInfo;

Timing::Entry::~Entry() noexcept
{
  if (info_owned())
    delete get_info();

  // 无需原子操作，因为析构操作只发生在最后持有 shared_ptr 的单个线程中。
  auto* next = mNext.load(std::memory_order_relaxed);
  if (next) {
    while (next != nullptr) {
      auto* nextnext = next->mNext.load(std::memory_order_relaxed);
      next->mNext.store(nullptr, std::memory_order_relaxed);
      delete next; // 先将 next 置空，防止陷入递归析构。
      next = nextnext;
    }
  }
}

std::string
Timing::Scope::EnterInfo::info() noexcept
{
  return "ENTER";
}

std::string
Timing::Scope::LeaveInfo::info() noexcept
{
  return "LEAVE";
}

} // namespace My
