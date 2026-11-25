#pragma once

#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> swizzle(const vector<T, N>& self, const vector<T, N>& src) {
    std::array<T, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    std::array<T, N> src_raw = src.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = src_raw[self_raw[i] % N];
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::swizzle(const vector<T, N>& src) const {
#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 128) {
#if LPS2_ARCH_AVX512VBMI
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm_maskz_permutexvar_epi8(_knot_mask16(_mm_movepi8_mask(raw)), raw, src.raw) };
      }
#endif
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm_maskz_permutexvar_epi16(_knot_mask8(_mm_movepi16_mask(raw)), raw, src.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm_maskz_permutexvar_epi32(_knot_mask8(_mm_movepi32_mask(raw)), raw, src.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm_maskz_permutexvar_epi64(_knot_mask8(_mm_movepi64_mask(raw)), raw, src.raw) };
      }
    }
    if constexpr (bitsize == 256) {
#if LPS2_ARCH_AVX512VBMI
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm256_maskz_permutexvar_epi8(_knot_mask32(_mm256_movepi8_mask(raw)), raw, src.raw) };
      }
#endif
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm256_maskz_permutexvar_epi16(_knot_mask16(_mm256_movepi16_mask(raw)), raw, src.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm256_maskz_permutexvar_epi32(_knot_mask8(_mm256_movepi32_mask(raw)), raw, src.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm256_maskz_permutexvar_epi64(_knot_mask8(_mm256_movepi64_mask(raw)), raw, src.raw) };
      }
    }
    if constexpr (bitsize == 512) {
#if LPS2_ARCH_AVX512VBMI
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm512_maskz_permutexvar_epi8(_knot_mask64(_mm512_movepi8_mask(raw)), raw, src.raw) };
      }
#endif
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm512_maskz_permutexvar_epi16(_knot_mask32(_mm512_movepi16_mask(raw)), raw, src.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm512_maskz_permutexvar_epi32(_knot_mask16(_mm512_movepi32_mask(raw)), raw, src.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm512_maskz_permutexvar_epi64(_knot_mask8(_mm512_movepi64_mask(raw)), raw, src.raw) };
      }
    }
#endif

#if LPS2_ARCH_SSSE3
    if constexpr (bitsize == 128 && sizeof(T) == sizeof(u8)) {
      return vector { _mm_shuffle_epi8(src.raw, raw) };
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256 && sizeof(T) == sizeof(u8)) {
      __m256i mask = _mm256_slli_epi16(raw, 3);
      __m256i src_lo = _mm256_permute2x128_si256(src.raw, src.raw, 0x00);
      __m256i src_hi = _mm256_permute2x128_si256(src.raw, src.raw, 0x11);
      return vector { _mm256_blendv_epi8(_mm256_shuffle_epi8(src_lo, raw), _mm256_shuffle_epi8(src_hi, raw), mask) };
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512 && sizeof(T) == sizeof(u8)) {
      using Half = vector<T, N / 2>;
      auto [raw0, raw1] = to_halves();
      auto [src0, src1] = src.to_halves();
      return concat(Half { raw0 }.swizzle(src0, src1).raw, Half { raw1 }.swizzle(src0, src1).raw);
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.swizzle(src.raw[0], src.raw[1]).raw;
      result.raw[1] = Half { raw[1] }.swizzle(src.raw[0], src.raw[1]).raw;
      return result;
    }

    return generic::vector_ops::swizzle(*this, src);
  }
}  // namespace lps2
