#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr T reduce_or(const vector<T, N>& self) {
    T result = 0;
    std::array<T, N> self_raw = self.to_array();
    for (usize i = 0; i < N; i++) {
      result |= self_raw[i];
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr T vector<T, N>::reduce_or() const {
#if LPS2_ARCH_X86_64
    if constexpr (sizeof(T) == sizeof(u64) && bitsize == 128) {
      __m128i r = raw;
      r = _mm_or_si128(r, _mm_unpackhi_epi64(r, r));
      return static_cast<u64>(_mm_extract_epi64(r, 0));
    }
#endif
#if LPS2_ARCH_AVX2
    if constexpr (sizeof(T) == sizeof(u64) && bitsize == 256) {
      __m128i r;
      r = _mm_or_si128(_mm256_extracti128_si256(raw, 1), _mm256_castsi256_si128(raw));
      r = _mm_or_si128(r, _mm_unpackhi_epi64(r, r));
      return static_cast<u64>(_mm_extract_epi64(r, 0));
    }
#endif
    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      return (Half { raw[0] } | Half { raw[1] }).reduce_or();
    }
    return generic::vector_ops::reduce_or(*this);
  }
}  // namespace lps2
