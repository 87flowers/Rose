#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N> basic_vector_mask<T, N>::andnot(const basic_vector_mask<T, N>& second) const {
    basic_vector_mask<T, N> result;
    result.raw = raw.andnot(second.raw);
    return result;
  }
}  // namespace lps2
