#pragma once

#include "lps2/bit_mask.hpp"
#include "lps2/detail/vector_arch_type.hpp"
#include "lps2/vector_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  template<class V>
    requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
  LPS2_INLINE constexpr V basic_bit_mask<T, N>::select(const V& v0, const V& v1) const {
#if LPS2_ARCH_AVX512BASE
    if constexpr (V::bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm_mask_blend_epi8(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm_mask_blend_epi16(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm_mask_blend_epi32(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm_mask_blend_epi64(raw, v0.raw, v1.raw) };
      }
    }
    if constexpr (V::bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm256_mask_blend_epi8(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm256_mask_blend_epi16(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm256_mask_blend_epi32(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm256_mask_blend_epi64(raw, v0.raw, v1.raw) };
      }
    }
    if constexpr (V::bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm512_mask_blend_epi8(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm512_mask_blend_epi16(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm512_mask_blend_epi32(raw, v0.raw, v1.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm512_mask_blend_epi64(raw, v0.raw, v1.raw) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<typename V::element_type, N>::is_doubled) {
      using Half = vector<typename V::element_type, N / 2>;
      auto [m0, m1] = to_halves();
      V result;
      result.raw[0] = m0.select(Half { v0.raw[0] }, Half { v1.raw[0] }).raw;
      result.raw[1] = m1.select(Half { v0.raw[1] }, Half { v1.raw[1] }).raw;
      return result;
    }

    return to_vm().select(v0, v1);
  }
}  // namespace lps2
