#pragma once

#include "lps2/detail/msb.hpp"
#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>
#include <climits>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> msb_bm(const vector<T, N>& first) {
    bit_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, detail::msb(first_raw[i]));
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::msb_bm() const {
#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm_movepi8_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm_movepi16_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm_movepi32_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm_movepi64_mask(raw) };
      }
    }
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm256_movepi8_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm256_movepi16_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm256_movepi32_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm256_movepi64_mask(raw) };
      }
    }
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm512_movepi8_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm512_movepi16_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm512_movepi32_mask(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm512_movepi64_mask(raw) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      return bit_mask<T, N>::concat(Half { raw[0] }.msb(), Half { raw[1] }.msb());
    }

    constexpr T msb_bit = static_cast<T>(1) << (sizeof(T) * CHAR_BIT - 1);
    return test_bm(vector::splat(msb_bit));
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::msb_vm() const {
    return msb_bm().to_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::msb() const {
    return msb_bm();
  }
}  // namespace lps2
