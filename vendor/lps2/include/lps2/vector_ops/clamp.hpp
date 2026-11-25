#pragma once

#include "lps2/vector.hpp"

#include <algorithm>
#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> clamp(const vector<T, N>& self, const vector<T, N>& min, const vector<T, N>& max) {
    std::array<T, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    std::array<T, N> min_raw = min.to_array();
    std::array<T, N> max_raw = max.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = std::clamp<T>(self_raw[i], min_raw[i], max_raw[i]);
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::clamp(const vector<T, N>& min, const vector<T, N>& max) const {
#if LPS2_ARCH_SSE4_1
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vector { _mm_min_epi8(_mm_max_epi8(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vector { _mm_min_epi16(_mm_max_epi16(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vector { _mm_min_epi32(_mm_max_epi32(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vector { _mm_min_epu8(_mm_max_epu8(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vector { _mm_min_epu16(_mm_max_epu16(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vector { _mm_min_epu32(_mm_max_epu32(raw, min.raw), max.raw) };
      }
    }
#endif
#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      if constexpr (std::is_same_v<T, i8>) {
        return vector { _mm256_min_epi8(_mm256_max_epi8(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vector { _mm256_min_epi16(_mm256_max_epi16(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vector { _mm256_min_epi32(_mm256_max_epi32(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vector { _mm256_min_epu8(_mm256_max_epu8(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vector { _mm256_min_epu16(_mm256_max_epu16(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vector { _mm256_min_epu32(_mm256_max_epu32(raw, min.raw), max.raw) };
      }
    }
#endif
#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      if constexpr (std::is_same_v<T, i8>) {
        return vector { _mm512_min_epi8(_mm512_max_epi8(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vector { _mm512_min_epi16(_mm512_max_epi16(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vector { _mm512_min_epi32(_mm512_max_epi32(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vector { _mm512_min_epi64(_mm512_max_epi64(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vector { _mm512_min_epu8(_mm512_max_epu8(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vector { _mm512_min_epu16(_mm512_max_epu16(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vector { _mm512_min_epu32(_mm512_max_epu32(raw, min.raw), max.raw) };
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vector { _mm512_min_epu64(_mm512_max_epu64(raw, min.raw), max.raw) };
      }
    }
#endif
    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.clamp(min.raw[0], max.raw[0]).raw;
      result.raw[1] = Half { raw[1] }.clamp(min.raw[1], max.raw[1]).raw;
      return result;
    }
    return generic::vector_ops::clamp(*this, min, max);
  }
}  // namespace lps2
