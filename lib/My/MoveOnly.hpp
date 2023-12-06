#pragma once

namespace My {

/**
 * @brief 仅支持移动语义的混入类。
 */
class MoveOnly
{
public:
  MoveOnly() = default;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly& operator=(MoveOnly&&) = default;
};

} // namespace My
