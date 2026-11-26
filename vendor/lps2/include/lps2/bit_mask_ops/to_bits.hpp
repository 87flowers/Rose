#pragma once

#include "lps2/bit_mask.hpp"

#include <bit>

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr basic_bit_mask<T, N>::bits_type basic_bit_mask<T, N>::to_bits() const {
    return std::bit_cast<bits_type>(raw);
  }
}  // namespace lps2
