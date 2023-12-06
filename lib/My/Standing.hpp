#pragma once

namespace My {

/**
 * @brief 不可复制、不可移动的混入类。
 */
class Standing
{
public:
  Standing() = default;
  Standing(const Standing&) = delete;
  Standing(Standing&&) = delete;
  Standing& operator=(const Standing&) = delete;
  Standing& operator=(Standing&&) = delete;
};

} // namespace My
