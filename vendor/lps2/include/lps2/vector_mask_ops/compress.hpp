#pragma once

#include "lps2/detail/mask_element.hpp"
#include "lps2/stdint.hpp"
#include "lps2/vector_mask.hpp"

#include <bit>
#include <type_traits>

namespace lps2::generic::vector_mask_ops {
  template<class T, usize N, class U>
    requires std::is_same_v<T, detail::mask_element_t<U>>
  LPS2_INLINE constexpr vector<U, N> compress(const basic_vector_mask<T, N>& self, const vector<U, N>& v) {
    auto src_array = v.to_array();
    auto mask_bits = self.to_bits();

    std::array<U, N> result;
    size_t write_idx = 0;

    // Bit scan loop
    while (mask_bits) {
      size_t bit_pos = std::countr_zero(mask_bits);
      result[write_idx++] = src_array[bit_pos];
      mask_bits &= mask_bits - 1;
    }

    // Zero tail
    for (size_t i = write_idx; i < N; ++i) {
      result[i] = U {};
    }

    return std::bit_cast<vector<U, N>>(result);
  }
}  // namespace lps2::generic::vector_mask_ops

namespace lps2 {
  template<class T, usize N>
  template<class V>
    requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
  LPS2_INLINE constexpr V basic_vector_mask<T, N>::compress(const V& v) const {
    return generic::vector_mask_ops::compress(*this, v);
  }
}  // namespace lps2
