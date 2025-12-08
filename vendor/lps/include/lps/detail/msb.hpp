#pragma once

#include "lps/stdint.hpp"

#include <climits>

namespace lps::detail {

  template<class T>
  bool msb(T x) {
    constexpr usize bits = sizeof(T) * CHAR_BIT;
    return x >> (bits - 1);
  }

}  // namespace lps::detail
