#pragma once

#include "lps2/bit_mask.hpp"

namespace lps2 {

  template<class T, usize N>
  LPS2_INLINE constexpr basic_bit_mask<T, N> operator|(const basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second) {
    using raw_type = typename basic_bit_mask<T, N>::raw_type;
#if LPS2_ARCH_AVX512BASE
    if constexpr (sizeof(raw_type) == sizeof(u8)) {
      return basic_bit_mask<T, N> { _kor_mask8(first.raw, second.raw) };
    }
    if constexpr (sizeof(raw_type) == sizeof(u16)) {
      return basic_bit_mask<T, N> { _kor_mask16(first.raw, second.raw) };
    }
    if constexpr (sizeof(raw_type) == sizeof(u32)) {
      return basic_bit_mask<T, N> { _kor_mask32(first.raw, second.raw) };
    }
    if constexpr (sizeof(raw_type) == sizeof(u64)) {
      return basic_bit_mask<T, N> { _kor_mask64(first.raw, second.raw) };
    }
#endif
    return basic_bit_mask<T, N> { static_cast<typename basic_bit_mask<T, N>::bits_type>(first.to_bits() | second.to_bits()) };
  }

  template<class T, usize N>
  LPS2_INLINE constexpr basic_bit_mask<T, N>& operator|=(basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second) {
    return first = first | second;
  }

}  // namespace lps2
