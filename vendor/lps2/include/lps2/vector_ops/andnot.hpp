#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>
#include <bit>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> andnot(const vector<T, N>& first, const vector<T, N>& second) {
    std::array<T, N> result_raw;
    std::array<T, N> first_raw = first.to_array();
    std::array<T, N> second_raw = second.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = first_raw[i] & ~second_raw[i];
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::andnot(const vector<T, N>& second) const {
#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      return vector { _mm_andnot_si128(second.raw, raw) };
    }
#endif
#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      return vector { _mm256_andnot_si256(second.raw, raw) };
    }
#endif
#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      return vector { _mm512_andnot_si512(second.raw, raw) };
    }
#endif
    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.andnot(second.raw[0]).raw;
      result.raw[1] = Half { raw[1] }.andnot(second.raw[1]).raw;
      return result;
    }
    return *this & ~second;
  }
}  // namespace lps2
