#pragma once

#include "lps2/vector.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::swizzle(const bit_mask<T, N>& src) const {
    return swizzle(src.to_vector().template convert<T>()).msb_bm();
  }
}  // namespace lps2
