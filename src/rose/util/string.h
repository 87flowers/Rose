#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rose/util/types.h"

namespace rose {

  auto stringSplit(std::string text, char delim) -> std::vector<std::string>;

  auto parseInt(std::string_view str) -> std::optional<int>;
  auto parseU32(std::string_view str) -> std::optional<u32>;
  auto parseU64(std::string_view str) -> std::optional<u64>;

} // namespace rose
