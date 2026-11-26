#pragma once

#include "lps2/bit_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr void basic_bit_mask<T, N>::set(usize index, bool value) {
    auto bit1 = static_cast<raw_type>(static_cast<bits_type>(1) << index);
    auto bitv = static_cast<raw_type>(static_cast<bits_type>(value) << index);
    raw = (andnot(basic_bit_mask { bit1 }) | basic_bit_mask { bitv }).raw;
  }
}  // namespace lps2
