#pragma once

#include "lps2/bit_mask.hpp"
#include "lps2/detail/vector_clamped_size.hpp"

namespace lps2 {
  template<class T, usize N>
  template<class U>
  LPS2_INLINE constexpr bit_mask<U, detail::clamped_size<U, N>> basic_bit_mask<T, N>::convert() const {
    return bit_mask<U, detail::clamped_size<U, N>> { raw };
  }
}  // namespace lps2
