#pragma once

#include "lps2/bit_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr basic_bit_mask<T, N> basic_bit_mask<T, N>::splat(bool value) {
    return value ? ~zero() : zero();
  }
}  // namespace lps2
