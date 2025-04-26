#pragma once

#include "rose/util/types.h"

namespace rose::bch {
  template <usize n> constexpr auto bitReverse(unsigned _BitInt(n) a) -> unsigned _BitInt(n) {
    unsigned _BitInt(n) result = 0;
    for (usize i = 0; i < n; i++) {
      if ((a >> i) & 1)
        result |= static_cast<unsigned _BitInt(n)>(1) << (n - i - 1);
    }
    return result;
  }

  template <usize n> constexpr auto ctz(unsigned _BitInt(n) a) -> usize {
    unsigned _BitInt(n) result = 0;
    for (usize i = 0; i < n; i++) {
      if ((a >> i) & 1)
        return i;
    }
    return n;
  }

  template <usize n> constexpr auto clz(unsigned _BitInt(n) a) -> usize {
    unsigned _BitInt(n) result = 0;
    for (usize i = 0; i < n; i++) {
      if ((a >> (n - i - 1)) & 1)
        return i;
    }
    return n;
  }

  template <usize n> constexpr auto bitWidth(unsigned _BitInt(n) a) -> usize { return n - clz(a); }
} // namespace rose::bch
