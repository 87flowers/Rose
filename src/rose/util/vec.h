#pragma once

#include <array>
#include <x86intrin.h>

#include "rose/util/types.h"

namespace rose::vec {

  struct v128 {
    __m128i raw;

    static auto fromArray(std::array<u8, 16> src) -> v128 { return {_mm_loadu_epi32(src.data())}; }
    static auto broadcast8(u8 src) -> v128 { return {_mm_set1_epi8(src)}; }
  };

  struct v256 {
    __m256i raw;

    static auto fromArray(std::array<u8, 32> src) -> v256 { return {_mm256_loadu_epi32(src.data())}; }
    static auto broadcast8(u8 src) -> v256 { return {_mm256_set1_epi8(src)}; }
  };

  struct v512 {
    __m512i raw;

    static auto fromArray(std::array<u8, 64> src) -> v512 { return {_mm512_loadu_epi32(src.data())}; }
    static auto from256(v256 src) -> v512 { return {_mm512_castsi256_si512(src.raw)}; }
    static auto broadcast8(u8 src) -> v512 { return {_mm512_set1_epi8(src)}; }

    auto nonzero8() const -> u64 { return _mm512_cmpneq_epu8_mask(raw, _mm512_setzero_si512()); }
  };

  inline auto add8(v128 a, v128 b) -> v128 { return {_mm_add_epi8(a.raw, b.raw)}; }
  inline auto add8(v256 a, v256 b) -> v256 { return {_mm256_add_epi8(a.raw, b.raw)}; }
  inline auto add8(v512 a, v512 b) -> v512 { return {_mm512_add_epi8(a.raw, b.raw)}; }

  inline auto permute8(v512 index, v512 a, v512 b) -> v512 { return {_mm512_permutex2var_epi8(a.raw, index.raw, b.raw)}; }

  inline auto testn8(v128 a, v128 b) -> u16 { return {_mm_testn_epi8_mask(a.raw, b.raw)}; }
  inline auto testn8(v256 a, v256 b) -> u32 { return {_mm256_testn_epi8_mask(a.raw, b.raw)}; }
  inline auto testn8(v512 a, v512 b) -> u64 { return {_mm512_testn_epi8_mask(a.raw, b.raw)}; }

} // namespace rose::vec
