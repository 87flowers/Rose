#pragma once

#include "lps2/arch.hpp"
#include "lps2/detail/vector_arch_type.hpp"
#include "lps2/vector_mask.hpp"

#include <array>
#include <bit>

namespace lps2::generic::vector_mask_ops {
  template<class T, usize N, class U>
    requires std::is_same_v<T, detail::mask_element_t<U>>
  LPS2_INLINE constexpr vector<U, N> select(const basic_vector_mask<T, N>& self, const vector<U, N>& v0, const vector<U, N>& v1) {
    std::array<U, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    std::array<U, N> v0_raw = v0.to_array();
    std::array<U, N> v1_raw = v1.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = self_raw[i] == basic_vector_mask<T, N>::false_value ? v0_raw[i] : v1_raw[i];
    }
    return std::bit_cast<vector<U, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_mask_ops

namespace lps2 {
  template<class T, usize N>
  template<class V>
    requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
  LPS2_INLINE constexpr V basic_vector_mask<T, N>::select(const V& v0, const V& v1) const {
#if LPS2_ARCH_SSE4_1
    if constexpr (inner_type::bitsize == 128) {
      return V { _mm_blendv_epi8(v0.raw, v1.raw, raw.raw) };
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (inner_type::bitsize == 256) {
      return V { _mm256_blendv_epi8(v0.raw, v1.raw, raw.raw) };
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (inner_type::bitsize == 512) {
      return V { _mm512_mask_blend_epi8(_mm512_movepi8_mask(raw.raw), v0.raw, v1.raw) };
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      const auto [v00, v01] = v0.to_halves();
      const auto [v10, v11] = v1.to_halves();
      V result;
      result.raw[0] = std::bit_cast<basic_vector_mask<T, N / 2>>(raw.raw[0]).select(v00, v10).raw;
      result.raw[1] = std::bit_cast<basic_vector_mask<T, N / 2>>(raw.raw[1]).select(v01, v11).raw;
      return result;
    }

    return generic::vector_mask_ops::select(*this, v0, v1);
  }
}  // namespace lps2
