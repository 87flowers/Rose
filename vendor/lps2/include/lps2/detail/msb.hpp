#pragma once

#include "lps2/stdint.hpp"

#include <climits>

namespace lps2::detail {
  template<class T>
  bool msb(T x) {
    constexpr usize bits = sizeof(T) * CHAR_BIT;
    return x >> (bits - 1);
  }
}  // namespace lps2::detail
