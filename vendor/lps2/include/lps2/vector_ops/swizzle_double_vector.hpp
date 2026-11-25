#pragma once

#include "lps2/detail/msb.hpp"
#include "lps2/vector.hpp"

#include <array>
#include <bit>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> swizzle(const vector<T, N>& self, const vector<T, N>& src0, const vector<T, N>& src1) {
    std::array<T, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    std::array<T, N> src0_raw = src0.to_array();
    std::array<T, N> src1_raw = src1.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = detail::msb(self_raw[i]) ? 0 : ((self_raw[i] & N) ? src1_raw : src0_raw)[self_raw[i] % N];
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::swizzle(const vector<T, N>& src0, const vector<T, N>& src1) const {
#if LPS2_ARCH_SSE4_1
    if constexpr (bitsize == 128 && sizeof(T) == sizeof(u8)) {
      __m128i mask = _mm_slli_epi16(raw, 3);
      return vector { _mm_blendv_epi8(_mm_shuffle_epi8(src0.raw, raw), _mm_shuffle_epi8(src1.raw, raw), mask) };
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256 && sizeof(T) == sizeof(u8)) {
      __m256i mask = _mm256_slli_epi16(raw, 2);
      return vector { _mm256_blendv_epi8(swizzle(src0).raw, swizzle(src1).raw, mask) };
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm_maskz_permutex2var_epi8(_knot_mask16(_mm_movepi8_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm_maskz_permutex2var_epi16(_knot_mask8(_mm_movepi16_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm_maskz_permutex2var_epi32(_knot_mask8(_mm_movepi32_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm_maskz_permutex2var_epi64(_knot_mask8(_mm_movepi64_mask(raw)), src0.raw, raw, src1.raw) };
      }
    }
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm256_maskz_permutex2var_epi8(_knot_mask32(_mm256_movepi8_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm256_maskz_permutex2var_epi16(_knot_mask16(_mm256_movepi16_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm256_maskz_permutex2var_epi32(_knot_mask8(_mm256_movepi32_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm256_maskz_permutex2var_epi64(_knot_mask8(_mm256_movepi64_mask(raw)), src0.raw, raw, src1.raw) };
      }
    }
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm512_maskz_permutex2var_epi8(_knot_mask64(_mm512_movepi8_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm512_maskz_permutex2var_epi16(_knot_mask32(_mm512_movepi16_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm512_maskz_permutex2var_epi32(_knot_mask16(_mm512_movepi32_mask(raw)), src0.raw, raw, src1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm512_maskz_permutex2var_epi64(_knot_mask8(_mm512_movepi64_mask(raw)), src0.raw, raw, src1.raw) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      static_assert(std::has_single_bit(N));

      auto mask0 = test(vector<T, N>::splat(N));
      auto index = andnot(vector<T, N>::splat(N));
      return mask0.select(index.swizzle(src0), index.swizzle(src1));
    }

    return generic::vector_ops::swizzle(*this, src0, src1);
  }
}  // namespace lps2
