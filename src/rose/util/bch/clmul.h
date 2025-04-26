#pragma once

#include "rose/util/types.h"

namespace rose::bch {
  template <usize n> constexpr auto clmul(unsigned _BitInt(n) a, unsigned _BitInt(n) b) -> unsigned _BitInt(n) {
    unsigned _BitInt(n) result = 0;
    for (usize i = 0; i < n; i++) {
      result <<= 1;
      if ((b >> (n - i - 1)) & 1)
        result ^= a;
    }
    return result;
  }
} // namespace rose::bch
