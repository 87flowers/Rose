#pragma once

#include "lps2/bit_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr basic_bit_mask<T, N> basic_bit_mask<T, N>::andnot(const basic_bit_mask<T, N>& second) const {
#if LPS2_ARCH_AVX512BASE
    if constexpr (sizeof(raw_type) == sizeof(u8)) {
      return basic_bit_mask { _kandn_mask8(second.raw, raw) };
    }
    if constexpr (sizeof(raw_type) == sizeof(u16)) {
      return basic_bit_mask { _kandn_mask16(second.raw, raw) };
    }
    if constexpr (sizeof(raw_type) == sizeof(u32)) {
      return basic_bit_mask { _kandn_mask32(second.raw, raw) };
    }
    if constexpr (sizeof(raw_type) == sizeof(u64)) {
      return basic_bit_mask { _kandn_mask64(second.raw, raw) };
    }
#endif
    return basic_bit_mask { static_cast<bits_type>(to_bits() & ~second.to_bits()) };
  }

}  // namespace lps2
