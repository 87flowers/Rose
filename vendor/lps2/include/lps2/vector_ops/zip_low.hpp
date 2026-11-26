#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>
#include <bit>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> zip_low(const vector<T, N>& self, const vector<T, N>& second) {
    static_assert(N % 2 == 0);
    std::array<T, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    std::array<T, N> second_raw = second.to_array();
    for (usize i = 0; i < N; i += 2) {
      result_raw[i + 0] = self_raw[i / 2];
      result_raw[i + 1] = second_raw[i / 2];
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::zip_low(const vector<T, N>& second) const {
#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm_unpacklo_epi8(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm_unpacklo_epi16(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm_unpacklo_epi32(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm_unpacklo_epi64(raw, second.raw) };
      }
    }
#endif

#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vmask_type { vzip1q_s8(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vmask_type { vzip1q_s16(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vmask_type { vzip1q_s32(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vmask_type { vzip1q_s64(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vmask_type { vzip1q_u8(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vmask_type { vzip1q_u16(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vmask_type { vzip1q_u32(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vmask_type { vzip1q_u64(raw, second.raw) };
      }
    }
#endif

    return generic::vector_ops::zip_low(*this, second);
  }
}  // namespace lps2
