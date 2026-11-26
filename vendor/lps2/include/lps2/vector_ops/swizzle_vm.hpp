#pragma once

#include "lps2/vector.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::swizzle(const vector_mask<T, N>& src) const {
    return vector_mask<T, N> { swizzle(vector<T, N> { src.raw.raw }).raw };
  }
}  // namespace lps2
