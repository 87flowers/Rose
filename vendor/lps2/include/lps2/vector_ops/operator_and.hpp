#pragma once

#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> operator_and(const vector<T, N>& first, const vector<T, N>& second) {
    std::array<T, N> result_raw;
    std::array<T, N> first_raw = first.to_array();
    std::array<T, N> second_raw = second.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = first_raw[i] & second_raw[i];
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N>& operator&=(vector<T, N>& first, const vector<T, N>& second) {
    return first = first & second;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> operator&(const vector<T, N>& first, const vector<T, N>& second) {
#if LPS2_ARCH_X86_64
    if constexpr (vector<T, N>::bitsize == 128) {
      return vector<T, N> { _mm_and_si128(first.raw, second.raw) };
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (vector<T, N>::bitsize == 256) {
      return vector<T, N> { _mm256_and_si256(first.raw, second.raw) };
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (vector<T, N>::bitsize == 512) {
      return vector<T, N> { _mm512_and_si512(first.raw, second.raw) };
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = (Half { first.raw[0] } & Half { second.raw[0] }).raw;
      result.raw[1] = (Half { first.raw[1] } & Half { second.raw[1] }).raw;
      return result;
    }

    return generic::vector_ops::operator_and(first, second);
  }
}  // namespace lps2
