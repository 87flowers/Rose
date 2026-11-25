#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector_mask.hpp"

#include <array>
#include <bit>

namespace lps2::generic::vector_mask_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr usize popcount(const basic_vector_mask<T, N>& self) {
    usize result = 0;
    std::array<T, N> self_raw = self.to_array();
    for (usize i = 0; i < N; i++) {
      result += self_raw[i] != basic_vector_mask<T, N>::false_value;
    }
    return result;
  }
}  // namespace lps2::generic::vector_mask_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr usize basic_vector_mask<T, N>::popcount() const {
#if LPS2_ARCH_X86_64
    if constexpr (inner_type::bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<usize>(std::popcount(static_cast<u16>(_mm_movemask_epi8(raw.raw))));
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<usize>(std::popcount(static_cast<u16>(_mm_movemask_epi8(raw.raw) & 0xAAAA)));
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm_movemask_ps((__m128)raw.raw))));
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm_movemask_pd((__m128d)raw.raw))));
      }
    }
#endif
#if LPS2_ARCH_AVX2
    if constexpr (inner_type::bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<usize>(std::popcount(static_cast<u32>(_mm256_movemask_epi8(raw.raw))));
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<usize>(std::popcount(static_cast<u32>(_mm256_movemask_epi8(raw.raw)) & 0xAAAAAAAA));
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm256_movemask_ps((__m256)raw.raw))));
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm256_movemask_pd((__m256d)raw.raw))));
      }
    }
#endif
#if LPS2_ARCH_AVX512BASE
    if constexpr (inner_type::bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<usize>(std::popcount(static_cast<u64>(_mm512_movepi8_mask(raw.raw))));
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<usize>(std::popcount(static_cast<u32>(_mm512_movepi16_mask(raw.raw))));
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<usize>(std::popcount(static_cast<u16>(_mm512_movepi32_mask(raw.raw))));
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm512_movepi64_mask(raw.raw))));
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector_mask<T, N / 2>;
      return Half { raw.raw[0] }.popcount() + Half { raw.raw[1] }.popcount();
    }

    return generic::vector_mask_ops::popcount(*this);
  }
}  // namespace lps2
