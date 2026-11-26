#pragma once

#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> operator_mul(const vector<T, N>& first, const vector<T, N>& second) {
    std::array<T, N> result_raw;
    std::array<T, N> first_raw = first.to_array();
    std::array<T, N> second_raw = second.to_array();

    for (usize i = 0; i < N; i++) {
      result_raw[i] = first_raw[i] * second_raw[i];
    }

    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N>& operator*=(vector<T, N>& first, const vector<T, N>& second) {
    return first = first * second;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> operator*(const vector<T, N>& first, const vector<T, N>& second) {
    constexpr usize bitsize = vector<T, N>::bitsize;

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N> { _mm512_mullo_epi16(first.raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N> { _mm512_mullo_epi32(first.raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N> { _mm512_mullo_epi64(first.raw, second.raw) };
      }
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N> { _mm256_mullo_epi16(first.raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N> { _mm256_mullo_epi32(first.raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N> { _mm256_mullo_epi64(first.raw, second.raw) };
      }
    }
#endif

#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N> { _mm_mullo_epi16(first.raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N> { _mm_mullo_epi32(first.raw, second.raw) };
      }
#if LPS2_ARCH_AVX512BASE
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N> { _mm_mullo_epi64(first.raw, second.raw) };
      }
#endif
    }
#endif

#if LPS2_ARCH_AARCH64
    if constexpr (bitsize == 128) {
      return vector<T, N> { first.raw * second.raw };
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = (Half { first.raw[0] } * Half { second.raw[0] }).raw;
      result.raw[1] = (Half { first.raw[1] } * Half { second.raw[1] }).raw;
      return result;
    }

    return generic::vector_ops::operator_mul(first, second);
  }
}  // namespace lps2
