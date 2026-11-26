#pragma once

#include "lps2/bit_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr basic_bit_mask<T, N> basic_bit_mask<T, N>::zero() {
    return basic_bit_mask { bits_type { 0 } };
  }
}  // namespace lps2
