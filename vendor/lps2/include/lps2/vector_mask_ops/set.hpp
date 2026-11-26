#pragma once

#include "lps2/vector_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr void basic_vector_mask<T, N>::set(usize index, bool value) {
    std::array<T, N> a = to_array();
    a[index] = value ? true_value : false_value;
    raw = std::bit_cast<inner_type>(a);
  }
}  // namespace lps2
