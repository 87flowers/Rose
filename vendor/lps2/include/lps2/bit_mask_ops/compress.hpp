#pragma once

#include "lps2/bit_mask.hpp"
#include "lps2/vector_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  template<class V>
    requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
  LPS2_INLINE constexpr V basic_bit_mask<T, N>::compress(const V& v) const {
#if LPS2_ARCH_AVX512VBMI2
#define LPS2_FAST_COMPRESS 1
    if constexpr (V::bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm_maskz_compress_epi8(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm_maskz_compress_epi16(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm_maskz_compress_epi32(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm_maskz_compress_epi64(raw, v.raw) };
      }
    }
    if constexpr (V::bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm256_maskz_compress_epi8(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm256_maskz_compress_epi16(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm256_maskz_compress_epi32(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm256_maskz_compress_epi64(raw, v.raw) };
      }
    }
    if constexpr (V::bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm512_maskz_compress_epi8(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm512_maskz_compress_epi16(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm512_maskz_compress_epi32(raw, v.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm512_maskz_compress_epi64(raw, v.raw) };
      }
    }
#endif

    return to_vm().compress(v);
  }
}  // namespace lps2
