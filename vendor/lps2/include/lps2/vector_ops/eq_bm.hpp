#pragma once

#include "lps2/detail/vector_arch_type.hpp"
#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> eq_bm(const vector<T, N>& first, const vector<T, N>& second) {
    bit_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    std::array<T, N> second_raw = second.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, first_raw[i] == second_raw[i]);
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::eq_bm(const vector<T, N>& second) const {
#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm_cmpeq_epu8_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm_cmpeq_epu16_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm_cmpeq_epu32_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm_cmpeq_epu64_mask(raw, second.raw) };
      }
    }
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm256_cmpeq_epu8_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm256_cmpeq_epu16_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm256_cmpeq_epu32_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm256_cmpeq_epu64_mask(raw, second.raw) };
      }
    }
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm512_cmpeq_epu8_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm512_cmpeq_epu16_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm512_cmpeq_epu32_mask(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm512_cmpeq_epu64_mask(raw, second.raw) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      return bit_mask<T, N>::concat(Half { raw[0] }.eq(second.raw[0]), Half { raw[1] }.eq(second.raw[1]));
    }

    return generic::vector_ops::eq_bm(*this, second);
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::eq_vm(const vector<T, N>& second) const {
    return eq_bm(second).to_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::eq(const vector<T, N>& second) const {
    return eq_bm(second);
  }
}  // namespace lps2
