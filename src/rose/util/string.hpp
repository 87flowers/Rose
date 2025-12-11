#pragma once

#include "rose/common.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rose {

  auto string_split(std::string text, char delim) -> std::vector<std::string>;

  auto parse_int(std::string_view str) -> std::optional<int>;
  auto parse_u16(std::string_view str) -> std::optional<u16>;
  auto parse_u32(std::string_view str) -> std::optional<u32>;
  auto parse_u64(std::string_view str) -> std::optional<u64>;

}  // namespace rose
