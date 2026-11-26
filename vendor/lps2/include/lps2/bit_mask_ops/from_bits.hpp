#pragma once

#include "lps2/bit_mask.hpp"

#include <bit>

namespace lps2 {

  template<class T, usize N>
  LPS2_INLINE constexpr basic_bit_mask<T, N>::basic_bit_mask(bits_type value)
    requires(!std::is_same_v<raw_type, bits_type>)
      :
      raw(std::bit_cast<raw_type>(value)) {
  }

}  // namespace lps2
