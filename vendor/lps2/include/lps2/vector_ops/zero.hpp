#pragma once

#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> zero() {
    std::array<T, N> result_raw;
    result_raw.fill(0);
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::zero() {
#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      return vector { _mm512_setzero_si512() };
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      return vector { _mm256_setzero_si256() };
    }
#endif

#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      return vector { _mm_setzero_si128() };
    }
#endif

#if LPS2_ARCH_AARCH64
    if constexpr (bitsize == 64 || bitsize == 128) {
      return splat(0);
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      const auto half = vector<T, N / 2>::zero();
      vector<T, N> result;
      result.raw[0] = half.raw;
      result.raw[1] = half.raw;
      return result;
    }

    return generic::vector_ops::zero<T, N>();
  }
}  // namespace lps2
