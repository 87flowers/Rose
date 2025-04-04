#pragma once

#include <array>
#include <x86intrin.h>

#include "rose/util/types.h"

namespace rose::vec {

  struct v128 {
    __m128i raw;

    using Mask8 = u16;

    forceinline static auto fromArray(std::array<u8, 16> src) -> v128 { return {_mm_loadu_epi32(src.data())}; }
    forceinline static auto broadcast8(u8 src) -> v128 { return {_mm_set1_epi8(src)}; }
    forceinline static auto broadcast64(u64 src) -> v128 { return {_mm_set1_epi64x(src)}; }

    forceinline auto msb8() const -> u16 { return _mm_movepi8_mask(raw); }

    forceinline constexpr auto operator==(const v128 &other) const -> bool {
      const __m128i t = _mm_xor_si128(raw, other.raw);
      return _mm_testz_si128(t, t);
    }
  };
  static_assert(sizeof(v128) == 16);

  struct v256 {
    __m256i raw;

    using Mask8 = u32;

    forceinline static auto fromArray(std::array<u8, 32> src) -> v256 { return {_mm256_loadu_epi32(src.data())}; }
    forceinline static auto broadcast8(u8 src) -> v256 { return {_mm256_set1_epi8(src)}; }
    forceinline static auto broadcast64(u64 src) -> v256 { return {_mm256_set1_epi64x(src)}; }

    forceinline auto msb8() const -> u32 { return _mm256_movepi8_mask(raw); }

    forceinline constexpr auto operator==(const v256 &other) const -> bool {
      const __m256i t = _mm256_xor_si256(raw, other.raw);
      return _mm256_testz_si256(t, t);
    }
  };
  static_assert(sizeof(v256) == 32);

  struct v512 {
    __m512i raw;

    using Mask8 = u64;

    forceinline static auto fromArray(std::array<u8, 64> src) -> v512 { return {_mm512_loadu_epi32(src.data())}; }
    forceinline static auto from256(v256 src) -> v512 { return {_mm512_castsi256_si512(src.raw)}; }
    forceinline static auto broadcast8(u8 src) -> v512 { return {_mm512_set1_epi8(src)}; }
    forceinline static auto broadcast64(u64 src) -> v512 { return {_mm512_set1_epi64(src)}; }

    forceinline auto msb8() const -> u64 { return _mm512_movepi8_mask(raw); }

    forceinline auto nonzero8() const -> u64 { return _mm512_cmpneq_epu8_mask(raw, _mm512_setzero_si512()); }

    forceinline constexpr auto operator==(const v512 &other) const -> bool { return _mm512_cmpeq_epu64_mask(raw, other.raw) == 0xFF; }
  };
  static_assert(sizeof(v512) == 64);

  forceinline auto add8(v128 a, v128 b) -> v128 { return {_mm_add_epi8(a.raw, b.raw)}; }
  forceinline auto add8(v256 a, v256 b) -> v256 { return {_mm256_add_epi8(a.raw, b.raw)}; }
  forceinline auto add8(v512 a, v512 b) -> v512 { return {_mm512_add_epi8(a.raw, b.raw)}; }

  forceinline auto and8(v128 a, v128 b) -> v128 { return {_mm_and_si128(a.raw, b.raw)}; }
  forceinline auto and8(v256 a, v256 b) -> v256 { return {_mm256_and_si256(a.raw, b.raw)}; }
  forceinline auto and8(v512 a, v512 b) -> v512 { return {_mm512_and_si512(a.raw, b.raw)}; }

  forceinline auto blend8(u16 mask, v128 a, v128 b) -> v128 { return {_mm_mask_blend_epi8(mask, a.raw, b.raw)}; }
  forceinline auto blend8(u32 mask, v256 a, v256 b) -> v256 { return {_mm256_mask_blend_epi8(mask, a.raw, b.raw)}; }
  forceinline auto blend8(u64 mask, v512 a, v512 b) -> v512 { return {_mm512_mask_blend_epi8(mask, a.raw, b.raw)}; }

  forceinline auto eq8(v128 a, v128 b) -> u16 { return {_mm_cmpeq_epu8_mask(a.raw, b.raw)}; }
  forceinline auto eq8(v256 a, v256 b) -> u32 { return {_mm256_cmpeq_epu8_mask(a.raw, b.raw)}; }
  forceinline auto eq8(v512 a, v512 b) -> u64 { return {_mm512_cmpeq_epu8_mask(a.raw, b.raw)}; }

  forceinline auto gf2p8affine8(v128 a, v128 b, u8 c) -> v128 { return {_mm_gf2p8affine_epi64_epi8(a.raw, b.raw, c)}; }
  forceinline auto gf2p8affine8(v256 a, v256 b, u8 c) -> v256 { return {_mm256_gf2p8affine_epi64_epi8(a.raw, b.raw, c)}; }
  forceinline auto gf2p8affine8(v512 a, v512 b, u8 c) -> v512 { return {_mm512_gf2p8affine_epi64_epi8(a.raw, b.raw, c)}; }

  forceinline auto permute8(v128 index, v128 a) -> v128 { return {_mm_permutexvar_epi8(index.raw, a.raw)}; }
  forceinline auto permute8(v256 index, v256 a) -> v256 { return {_mm256_permutexvar_epi8(index.raw, a.raw)}; }
  forceinline auto permute8(v512 index, v512 a) -> v512 { return {_mm512_permutexvar_epi8(index.raw, a.raw)}; }

  forceinline auto permute8(v128 index, v128 a, v128 b) -> v128 { return {_mm_permutex2var_epi8(a.raw, index.raw, b.raw)}; }
  forceinline auto permute8(v256 index, v256 a, v256 b) -> v256 { return {_mm256_permutex2var_epi8(a.raw, index.raw, b.raw)}; }
  forceinline auto permute8(v512 index, v512 a, v512 b) -> v512 { return {_mm512_permutex2var_epi8(a.raw, index.raw, b.raw)}; }

  forceinline auto sub8(v128 a, v128 b) -> v128 { return {_mm_sub_epi8(a.raw, b.raw)}; }
  forceinline auto sub8(v256 a, v256 b) -> v256 { return {_mm256_sub_epi8(a.raw, b.raw)}; }
  forceinline auto sub8(v512 a, v512 b) -> v512 { return {_mm512_sub_epi8(a.raw, b.raw)}; }

  forceinline auto test8(v128 a, v128 b) -> u16 { return _mm_test_epi8_mask(a.raw, b.raw); }
  forceinline auto test8(v256 a, v256 b) -> u32 { return _mm256_test_epi8_mask(a.raw, b.raw); }
  forceinline auto test8(v512 a, v512 b) -> u64 { return _mm512_test_epi8_mask(a.raw, b.raw); }

  forceinline auto testn8(v128 a, v128 b) -> u16 { return _mm_testn_epi8_mask(a.raw, b.raw); }
  forceinline auto testn8(v256 a, v256 b) -> u32 { return _mm256_testn_epi8_mask(a.raw, b.raw); }
  forceinline auto testn8(v512 a, v512 b) -> u64 { return _mm512_testn_epi8_mask(a.raw, b.raw); }

} // namespace rose::vec
