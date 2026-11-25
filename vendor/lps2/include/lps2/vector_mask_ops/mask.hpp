#pragma once

#include "lps2/vector_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  template<class V>
    requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
  LPS2_INLINE constexpr V basic_vector_mask<T, N>::mask(const V& v1) const {
    return V { raw.raw } & v1;
  }
}  // namespace lps2
