#pragma once

#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> splat(T value) {
    std::array<T, N> result_raw;
    result_raw.fill(value);
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::splat(T value) {
#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm512_set1_epi8(std::bit_cast<i8>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm512_set1_epi16(std::bit_cast<i16>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm512_set1_epi32(std::bit_cast<i32>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm512_set1_epi64(std::bit_cast<i64>(value)) };
      }
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm256_set1_epi8(std::bit_cast<i8>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm256_set1_epi16(std::bit_cast<i16>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm256_set1_epi32(std::bit_cast<i32>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm256_set1_epi64x(std::bit_cast<i64>(value)) };
      }
    }
#endif

#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm_set1_epi8(std::bit_cast<i8>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm_set1_epi16(std::bit_cast<i16>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm_set1_epi32(std::bit_cast<i32>(value)) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm_set1_epi64x(std::bit_cast<i64>(value)) };
      }
    }
#endif

#if LPS2_ARCH_AARCH64
    if constexpr (bitsize == 128) {
      if constexpr (std::same_as<T, i8>)
        return vector { vmovq_n_s8(value) };
      if constexpr (std::same_as<T, i16>)
        return vector { vmovq_n_s16(value) };
      if constexpr (std::same_as<T, i32>)
        return vector { vmovq_n_s32(value) };
      if constexpr (std::same_as<T, i64>)
        return vector { vmovq_n_s64(value) };
      if constexpr (std::same_as<T, u8>)
        return vector { vmovq_n_u8(value) };
      if constexpr (std::same_as<T, u16>)
        return vector { vmovq_n_u16(value) };
      if constexpr (std::same_as<T, u32>)
        return vector { vmovq_n_u32(value) };
      if constexpr (std::same_as<T, u64>)
        return vector { vmovq_n_u64(value) };
    }

    if constexpr (bitsize == 64) {
      if constexpr (std::same_as<T, i8>)
        return vector { vmov_n_s8(value) };
      if constexpr (std::same_as<T, i16>)
        return vector { vmov_n_s16(value) };
      if constexpr (std::same_as<T, i32>)
        return vector { vmov_n_s32(value) };
      if constexpr (std::same_as<T, i64>)
        return vector { vmov_n_s64(value) };
      if constexpr (std::same_as<T, u8>)
        return vector { vmov_n_u8(value) };
      if constexpr (std::same_as<T, u16>)
        return vector { vmov_n_u16(value) };
      if constexpr (std::same_as<T, u32>)
        return vector { vmov_n_u32(value) };
      if constexpr (std::same_as<T, u64>)
        return vector { vmov_n_u64(value) };
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      const auto half = vector<T, N / 2>::splat(value);
      vector<T, N> result;
      result.raw[0] = half.raw;
      result.raw[1] = half.raw;
      return result;
    }

    return generic::vector_ops::splat<T, N>(value);
  }
}  // namespace lps2
