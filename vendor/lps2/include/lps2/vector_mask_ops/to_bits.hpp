#pragma once

#include "lps2/detail/bit_mask_base.hpp"
#include "lps2/detail/vector_arch_type.hpp"
#include "lps2/stdint.hpp"
#include "lps2/vector_mask.hpp"

#include <array>

namespace lps2 {

  template<class T, usize N>
  LPS2_INLINE constexpr detail::bit_mask_base_t<N> basic_vector_mask<T, N>::to_bits() const {
#if LPS2_ARCH_X86_64
    if constexpr (inner_type::bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<u16>(_mm_movemask_epi8(raw.raw));
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<u8>(_mm_movemask_epi8(_mm_packs_epi16(raw.raw, _mm_setzero_si128())));
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<u8>(_mm_movemask_ps((__m128)raw.raw));
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<u8>(_mm_movemask_pd((__m128d)raw.raw));
      }
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (inner_type::bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<u32>(_mm256_movemask_epi8(raw.raw));
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<u16>(_mm_movemask_epi8(_mm_packs_epi16(_mm256_castsi256_si128(raw.raw), _mm256_extracti128_si256(raw.raw, 1))));
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<u8>(_mm256_movemask_ps((__m256)raw.raw));
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<u8>(_mm256_movemask_pd((__m256d)raw.raw));
      }
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (inner_type::bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<u64>(_mm512_movepi8_mask(raw.raw));
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<u32>(_mm512_movepi16_mask(raw.raw));
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<u16>(_mm512_movepi32_mask(raw.raw));
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<u8>(_mm512_movepi64_mask(raw.raw));
      }
    }
#endif

#if LPS2_ARCH_NEON
    if constexpr (inner_type::bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        vector<u8, 16> bits { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
        vector<u8, 16> masked = raw.template convert<u8>() & bits;
        u16 result = 0;
        result |= vaddv_u8(vget_low_u8(masked.raw));
        result |= static_cast<u16>(vaddv_u8(vget_high_u8(masked.raw))) << 8;
        return result;
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        vector<u16, 8> bits { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
        return vaddvq_u16((raw.template convert<u16>() & bits).raw);
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        vector<u32, 4> bits { 0x01, 0x02, 0x04, 0x08 };
        return vaddvq_u32((raw.template convert<u32>() & bits).raw);
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        vector<u64, 2> bits { 0x01, 0x02 };
        return vaddvq_u64((raw.template convert<u64>() & bits).raw);
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector_mask<T, N / 2>;
      detail::bit_mask_base_t<N> result = 0;
      result |= static_cast<detail::bit_mask_base_t<N>>(Half { raw.raw[0] }.to_bits());
      result |= static_cast<detail::bit_mask_base_t<N>>(Half { raw.raw[1] }.to_bits()) << (N / 2);
      return result;
    }

    detail::bit_mask_base_t<N> result = 0;
    std::array<T, N> self_raw = to_array();
    for (usize i = 0; i < N; i++) {
      detail::bit_mask_base_t<N> bit = self_raw[i] != false_value;
      result |= bit << i;
    }
    return result;
  }

}  // namespace lps2
