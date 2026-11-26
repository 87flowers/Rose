#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>
#include <bit>

namespace lps2::generic::vector_ops {
  template<usize shift_amount, class T, usize N>
  LPS2_INLINE constexpr vector<T, N> shl(const vector<T, N>& self) {
    std::array<T, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = self_raw[i] << shift_amount;
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  template<usize shift_amount>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::shl() const {
#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        constexpr u8 mask = static_cast<u8>(0xFF << shift_amount);
        return vector { _mm_slli_epi16(raw, shift_amount) } & vector::splat(mask);
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm_slli_epi16(raw, shift_amount) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm_slli_epi32(raw, shift_amount) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm_slli_epi64(raw, shift_amount) };
      }
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        constexpr u8 mask = static_cast<u8>(0xFF << shift_amount);
        return vector { _mm256_slli_epi16(raw, shift_amount) } & vector::splat(mask);
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm256_slli_epi16(raw, shift_amount) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm256_slli_epi32(raw, shift_amount) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm256_slli_epi64(raw, shift_amount) };
      }
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        constexpr u8 mask = static_cast<u8>(0xFF << shift_amount);
        return vector { _mm512_slli_epi16(raw, shift_amount) } & vector::splat(mask);
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm512_slli_epi16(raw, shift_amount) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm512_slli_epi32(raw, shift_amount) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm512_slli_epi64(raw, shift_amount) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.template shl<shift_amount>().raw;
      result.raw[1] = Half { raw[1] }.template shl<shift_amount>().raw;
      return result;
    }

    return generic::vector_ops::shl<shift_amount>(*this);
  }
}  // namespace lps2
