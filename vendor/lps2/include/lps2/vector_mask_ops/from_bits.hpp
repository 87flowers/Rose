#pragma once

#include "lps2/detail/bit_mask_base.hpp"
#include "lps2/stdint.hpp"
#include "lps2/vector_mask.hpp"

#include <bit>

namespace lps2 {

  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N>::basic_vector_mask(detail::bit_mask_base_t<N> value) {
    std::array<T, N> self_raw;
    for (usize i = 0; i < N; i++) {
      self_raw[i] = ((value >> i) & 1) ? true_value : false_value;
    }
    raw = std::bit_cast<inner_type>(self_raw);
  }

}  // namespace lps2
