#pragma once

#include "lps2/vector_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N> basic_vector_mask<T, N>::zero() {
    basic_vector_mask<T, N> result;
    result.raw = inner_type::zero();
    return result;
  }
}  // namespace lps2
