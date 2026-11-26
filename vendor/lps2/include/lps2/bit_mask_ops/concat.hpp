#pragma once

#include "lps2/bit_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr basic_bit_mask<T, N> basic_bit_mask<T, N>::concat(basic_bit_mask<T, N / 2> lower, basic_bit_mask<T, N / 2> upper) {
#if LPS2_ARCH_AVX512BASE
    if constexpr (N == 16) {
      return basic_bit_mask { _mm512_kunpackb(upper.raw, lower.raw) };
    }
    if constexpr (N == 32) {
      return basic_bit_mask { _mm512_kunpackw(upper.raw, lower.raw) };
    }
    if constexpr (N == 64) {
      return basic_bit_mask { _mm512_kunpackd(upper.raw, lower.raw) };
    }
#endif
    return basic_bit_mask { static_cast<bits_type>(static_cast<bits_type>(lower.to_bits()) | (static_cast<bits_type>(upper.to_bits()) << (N / 2))) };
  }
}  // namespace lps2
