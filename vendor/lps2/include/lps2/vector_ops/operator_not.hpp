#pragma once

#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> operator_not(const vector<T, N>& first) {
    std::array<T, N> result_raw;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = ~first_raw[i];
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> operator~(const vector<T, N>& first) {
#if LPS2_ARCH_X86_64
    if constexpr (vector<T, N>::bitsize == 128) {
      return vector<T, N> { _mm_xor_si128(first.raw, _mm_set1_epi32(-1)) };
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (vector<T, N>::bitsize == 256) {
      return vector<T, N> { _mm256_xor_si256(first.raw, _mm256_set1_epi32(-1)) };
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (vector<T, N>::bitsize == 512) {
      return vector<T, N> { _mm512_xor_si512(first.raw, _mm512_set1_epi32(-1)) };
    }
#endif

#if LPS2_ARCH_AARCH64
    if constexpr (vector<T, N>::bitsize == 128) {
      return vector<T, N> { ~first.raw };
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = (~Half { first.raw[0] }).raw;
      result.raw[1] = (~Half { first.raw[1] }).raw;
      return result;
    }

    return generic::vector_ops::operator_not(first);
  }
}  // namespace lps2
