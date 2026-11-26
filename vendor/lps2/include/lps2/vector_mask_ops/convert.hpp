#pragma once

#include "lps2/detail/vector_clamped_size.hpp"
#include "lps2/vector_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  template<class U>
  LPS2_INLINE constexpr basic_vector_mask<detail::mask_element_t<U>, detail::clamped_size<U, N>> basic_vector_mask<T, N>::convert() const {
    return vector_mask<U, detail::clamped_size<U, N>> { raw.template convert<detail::mask_element_t<U>>().raw };
  }
}  // namespace lps2
