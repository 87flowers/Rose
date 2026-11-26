#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>
#include <bit>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> zip_high_128bit(const vector<T, N>& self, const vector<T, N>& second) {
    static_assert(N % 2 == 0);
    std::array<T, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    std::array<T, N> second_raw = second.to_array();
    constexpr usize lane_width = 16 / sizeof(T);
    constexpr usize lane_count = N / lane_width;
    static_assert(lane_count * lane_width == N);
    for (usize lane = 0; lane < lane_count; lane++) {
      usize lane_start = lane * lane_width;
      for (usize i = 0; i < lane_width; i += 2) {
        result_raw[lane_start + i + 0] = self_raw[lane_start + (lane_width + i) / 2];
        result_raw[lane_start + i + 1] = second_raw[lane_start + (lane_width + i) / 2];
      }
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::zip_high_128bit(const vector<T, N>& second) const {
#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm_unpackhi_epi8(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm_unpackhi_epi16(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm_unpackhi_epi32(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm_unpackhi_epi64(raw, second.raw) };
      }
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm256_unpackhi_epi8(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm256_unpackhi_epi16(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm256_unpackhi_epi32(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm256_unpackhi_epi64(raw, second.raw) };
      }
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm512_unpackhi_epi8(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm512_unpackhi_epi16(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm512_unpackhi_epi32(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm512_unpackhi_epi64(raw, second.raw) };
      }
    }
#endif

#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vmask_type { vzip2q_s8(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vmask_type { vzip2q_s16(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vmask_type { vzip2q_s32(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vmask_type { vzip2q_s64(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vmask_type { vzip2q_u8(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vmask_type { vzip2q_u16(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vmask_type { vzip2q_u32(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vmask_type { vzip2q_u64(raw, second.raw) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.zip_high_128bit(second.raw[0]);
      result.raw[1] = Half { raw[1] }.zip_high_128bit(second.raw[1]);
      return result;
    }

    return generic::vector_ops::zip_high_128bit(*this, second);
  }
}  // namespace lps2
