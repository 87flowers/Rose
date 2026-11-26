#pragma once

#include "lps2/bit_mask.hpp"

namespace lps2 {

  template<class T, usize N>
  LPS2_INLINE constexpr bool operator==(const basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second) {
    return first.raw == second.raw;
  }

}  // namespace lps2
