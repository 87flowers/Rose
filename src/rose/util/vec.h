#pragma once

#include <array>
#include <bit>
#include <x86intrin.h>

#include "rose/util/types.h"

namespace rose::vec {

  struct v128 {
    __m128i raw;

    using Mask8 = u16;
    using Mask16 = u8;
    using Mask32 = u8;
    using Mask64 = u8;

    forceinline constexpr v128() = default;
    forceinline constexpr v128(__m128i raw) : raw(raw){};
    forceinline constexpr explicit v128(std::array<u8, 16> src) : raw(std::bit_cast<__m128i>(src)) {}
    forceinline constexpr explicit v128(std::array<u16, 8> src) : raw(std::bit_cast<__m128i>(src)) {}
    forceinline static auto load(const void *src) -> v128 { return {_mm_loadu_si128(reinterpret_cast<const __m128i *>(src))}; }
    forceinline static auto from32(u32 src) -> v128 { return {_mm_cvtsi32_si128(static_cast<i32>(src))}; }
    forceinline static auto from64(u64 src) -> v128 { return {_mm_cvtsi64_si128(static_cast<i64>(src))}; }
    forceinline static auto expandMask8(u16 src) -> v128 { return {_mm_movm_epi8(src)}; }
    forceinline static auto broadcast8(u8 src) -> v128 { return {_mm_set1_epi8(src)}; }
    forceinline static auto broadcast16(u16 src) -> v128 { return {_mm_set1_epi16(src)}; }
    forceinline static auto broadcast32(u32 src) -> v128 { return {_mm_set1_epi32(src)}; }
    forceinline static auto broadcast64(u64 src) -> v128 { return {_mm_set1_epi64x(src)}; }

    forceinline auto to32() const -> u32 { return static_cast<u32>(_mm_cvtsi128_si32(raw)); }
    forceinline auto to64() const -> u64 { return static_cast<u64>(_mm_cvtsi128_si64(raw)); }

    forceinline auto msb8() const -> u16 { return _mm_movepi8_mask(raw); }

    forceinline auto zero8() const -> u16 { return _mm_cmpeq_epu8_mask(raw, _mm_setzero_si128()); }
    forceinline auto nonzero8() const -> u16 { return _mm_cmpneq_epu8_mask(raw, _mm_setzero_si128()); }
    forceinline auto zero16() const -> u8 { return _mm_cmpeq_epu16_mask(raw, _mm_setzero_si128()); }
    forceinline auto nonzero16() const -> u8 { return _mm_cmpneq_epu16_mask(raw, _mm_setzero_si128()); }

    forceinline constexpr auto operator==(const v128 &other) const -> bool {
      const __m128i t = _mm_xor_si128(raw, other.raw);
      return _mm_testz_si128(t, t);
    }
  };
  static_assert(sizeof(v128) == 16);

  struct v256 {
    __m256i raw;

    using Mask8 = u32;
    using Mask16 = u16;
    using Mask32 = u8;
    using Mask64 = u8;

    forceinline constexpr v256() = default;
    forceinline constexpr v256(__m256i raw) : raw(raw){};
    forceinline constexpr explicit v256(std::array<u8, 32> src) : raw(std::bit_cast<__m256i>(src)) {}
    forceinline constexpr explicit v256(std::array<u16, 16> src) : raw(std::bit_cast<__m256i>(src)) {}
    forceinline static auto from128(v128 src) -> v256 { return {_mm256_castsi128_si256(src.raw)}; }
    forceinline static auto expandMask8(u32 src) -> v256 { return {_mm256_movm_epi8(src)}; }
    forceinline static auto broadcast8(u8 src) -> v256 { return {_mm256_set1_epi8(src)}; }
    forceinline static auto broadcast16(u16 src) -> v256 { return {_mm256_set1_epi16(src)}; }
    forceinline static auto broadcast32(u32 src) -> v256 { return {_mm256_set1_epi32(src)}; }
    forceinline static auto broadcast64(u64 src) -> v256 { return {_mm256_set1_epi64x(src)}; }

    forceinline auto to128() const -> v128 { return {_mm256_castsi256_si128(raw)}; }

    forceinline auto msb8() const -> u32 { return _mm256_movepi8_mask(raw); }

    forceinline auto zero8() const -> u32 { return _mm256_cmpeq_epu8_mask(raw, _mm256_setzero_si256()); }
    forceinline auto nonzero8() const -> u32 { return _mm256_cmpneq_epu8_mask(raw, _mm256_setzero_si256()); }
    forceinline auto zero16() const -> u16 { return _mm256_cmpeq_epu16_mask(raw, _mm256_setzero_si256()); }
    forceinline auto nonzero16() const -> u16 { return _mm256_cmpneq_epu16_mask(raw, _mm256_setzero_si256()); }

    forceinline constexpr auto operator==(const v256 &other) const -> bool {
      const __m256i t = _mm256_xor_si256(raw, other.raw);
      return _mm256_testz_si256(t, t);
    }
  };
  static_assert(sizeof(v256) == 32);

  struct v512 {
    __m512i raw;

    using Mask8 = u64;
    using Mask16 = u32;
    using Mask32 = u16;
    using Mask64 = u8;

    forceinline constexpr v512() = default;
    forceinline constexpr v512(__m512i raw) : raw(raw){};
    forceinline constexpr explicit v512(std::array<u8, 64> src) : raw(std::bit_cast<__m512i>(src)) {}
    forceinline constexpr explicit v512(std::array<u16, 32> src) : raw(std::bit_cast<__m512i>(src)) {}
    forceinline static auto from128(v128 src) -> v512 { return {_mm512_castsi128_si512(src.raw)}; }
    forceinline static auto from256(v256 src) -> v512 { return {_mm512_castsi256_si512(src.raw)}; }
    forceinline static auto expandMask8(u64 src) -> v512 { return {_mm512_movm_epi8(src)}; }
    forceinline static auto broadcast8(u8 src) -> v512 { return {_mm512_set1_epi8(src)}; }
    forceinline static auto broadcast16(u16 src) -> v512 { return {_mm512_set1_epi16(src)}; }
    forceinline static auto broadcast32(u32 src) -> v512 { return {_mm512_set1_epi32(src)}; }
    forceinline static auto broadcast64(u64 src) -> v512 { return {_mm512_set1_epi64(src)}; }

    forceinline auto to128() const -> v128 { return {_mm512_castsi512_si128(raw)}; }
    forceinline auto to256() const -> v256 { return {_mm512_castsi512_si256(raw)}; }

    forceinline auto msb8() const -> u64 { return _mm512_movepi8_mask(raw); }

    forceinline auto zero8() const -> u64 { return _mm512_cmpeq_epu8_mask(raw, _mm512_setzero_si512()); }
    forceinline auto nonzero8() const -> u64 { return _mm512_cmpneq_epu8_mask(raw, _mm512_setzero_si512()); }
    forceinline auto zero16() const -> u32 { return _mm512_cmpeq_epu16_mask(raw, _mm512_setzero_si512()); }
    forceinline auto nonzero16() const -> u32 { return _mm512_cmpneq_epu16_mask(raw, _mm512_setzero_si512()); }

    forceinline constexpr auto operator==(const v512 &other) const -> bool { return _mm512_cmpeq_epu64_mask(raw, other.raw) == 0xFF; }
  };
  static_assert(sizeof(v512) == 64);

  forceinline auto operator|(v128 a, v128 b) -> v128 { return {_mm_or_si128(a.raw, b.raw)}; }
  forceinline auto operator|(v256 a, v256 b) -> v256 { return {_mm256_or_si256(a.raw, b.raw)}; }
  forceinline auto operator|(v512 a, v512 b) -> v512 { return {_mm512_or_si512(a.raw, b.raw)}; }

  forceinline auto operator|=(v128 &a, v128 b) -> v128 & { return a = a | b; }
  forceinline auto operator|=(v256 &a, v256 b) -> v256 & { return a = a | b; }
  forceinline auto operator|=(v512 &a, v512 b) -> v512 & { return a = a | b; }

  forceinline auto operator&(v128 a, v128 b) -> v128 { return {_mm_and_si128(a.raw, b.raw)}; }
  forceinline auto operator&(v256 a, v256 b) -> v256 { return {_mm256_and_si256(a.raw, b.raw)}; }
  forceinline auto operator&(v512 a, v512 b) -> v512 { return {_mm512_and_si512(a.raw, b.raw)}; }

  forceinline auto operator&=(v128 &a, v128 b) -> v128 & { return a = a & b; }
  forceinline auto operator&=(v256 &a, v256 b) -> v256 & { return a = a & b; }
  forceinline auto operator&=(v512 &a, v512 b) -> v512 & { return a = a & b; }

  forceinline auto operator^(v128 a, v128 b) -> v128 { return {_mm_xor_si128(a.raw, b.raw)}; }
  forceinline auto operator^(v256 a, v256 b) -> v256 { return {_mm256_xor_si256(a.raw, b.raw)}; }
  forceinline auto operator^(v512 a, v512 b) -> v512 { return {_mm512_xor_si512(a.raw, b.raw)}; }

  forceinline auto operator^=(v128 &a, v128 b) -> v128 & { return a = a ^ b; }
  forceinline auto operator^=(v256 &a, v256 b) -> v256 & { return a = a ^ b; }
  forceinline auto operator^=(v512 &a, v512 b) -> v512 & { return a = a ^ b; }

  forceinline auto add8(v128 a, v128 b) -> v128 { return {_mm_add_epi8(a.raw, b.raw)}; }
  forceinline auto add8(v256 a, v256 b) -> v256 { return {_mm256_add_epi8(a.raw, b.raw)}; }
  forceinline auto add8(v512 a, v512 b) -> v512 { return {_mm512_add_epi8(a.raw, b.raw)}; }
  forceinline auto add16(v128 a, v128 b) -> v128 { return {_mm_add_epi16(a.raw, b.raw)}; }
  forceinline auto add16(v256 a, v256 b) -> v256 { return {_mm256_add_epi16(a.raw, b.raw)}; }
  forceinline auto add16(v512 a, v512 b) -> v512 { return {_mm512_add_epi16(a.raw, b.raw)}; }

  forceinline auto add8_sat(v128 a, v128 b) -> v128 { return {_mm_adds_epi8(a.raw, b.raw)}; }
  forceinline auto add8_sat(v256 a, v256 b) -> v256 { return {_mm256_adds_epi8(a.raw, b.raw)}; }
  forceinline auto add8_sat(v512 a, v512 b) -> v512 { return {_mm512_adds_epi8(a.raw, b.raw)}; }

  forceinline auto bitshuffle(v128 a, v128 b) -> u16 { return _mm_bitshuffle_epi64_mask(a.raw, b.raw); }
  forceinline auto bitshuffle(v256 a, v256 b) -> u32 { return _mm256_bitshuffle_epi64_mask(a.raw, b.raw); }
  forceinline auto bitshuffle(v512 a, v512 b) -> u64 { return _mm512_bitshuffle_epi64_mask(a.raw, b.raw); }

  forceinline auto bitshuffle_m(u16 m, v128 a, v128 b) -> u16 { return _mm_mask_bitshuffle_epi64_mask(m, a.raw, b.raw); }
  forceinline auto bitshuffle_m(u32 m, v256 a, v256 b) -> u32 { return _mm256_mask_bitshuffle_epi64_mask(m, a.raw, b.raw); }
  forceinline auto bitshuffle_m(u64 m, v512 a, v512 b) -> u64 { return _mm512_mask_bitshuffle_epi64_mask(m, a.raw, b.raw); }

  forceinline auto compress8(u16 mask, v128 a) -> v128 { return {_mm_maskz_compress_epi8(mask, a.raw)}; }
  forceinline auto compress8(u32 mask, v256 a) -> v256 { return {_mm256_maskz_compress_epi8(mask, a.raw)}; }
  forceinline auto compress8(u64 mask, v512 a) -> v512 { return {_mm512_maskz_compress_epi8(mask, a.raw)}; }
  forceinline auto compress16(u8 mask, v128 a) -> v128 { return {_mm_maskz_compress_epi16(mask, a.raw)}; }
  forceinline auto compress16(u16 mask, v256 a) -> v256 { return {_mm256_maskz_compress_epi16(mask, a.raw)}; }
  forceinline auto compress16(u32 mask, v512 a) -> v512 { return {_mm512_maskz_compress_epi16(mask, a.raw)}; }
  forceinline auto compress32(u8 mask, v128 a) -> v128 { return {_mm_maskz_compress_epi32(mask, a.raw)}; }
  forceinline auto compress32(u8 mask, v256 a) -> v256 { return {_mm256_maskz_compress_epi32(mask, a.raw)}; }
  forceinline auto compress32(u16 mask, v512 a) -> v512 { return {_mm512_maskz_compress_epi32(mask, a.raw)}; }
  forceinline auto compress64(u8 mask, v128 a) -> v128 { return {_mm_maskz_compress_epi64(mask, a.raw)}; }
  forceinline auto compress64(u8 mask, v256 a) -> v256 { return {_mm256_maskz_compress_epi64(mask, a.raw)}; }
  forceinline auto compress64(u8 mask, v512 a) -> v512 { return {_mm512_maskz_compress_epi64(mask, a.raw)}; }

  forceinline auto concatlo16(u16 a, u16 b) -> u16 { return _mm512_kunpackb(b, a); }
  forceinline auto concatlo32(u32 a, u32 b) -> u32 { return _mm512_kunpackw(b, a); }
  forceinline auto concatlo64(u64 a, u64 b) -> u64 { return _mm512_kunpackd(b, a); }

  forceinline auto compressstore16(void *dest, u8 mask, v128 a) -> void { _mm_mask_compressstoreu_epi16(dest, mask, a.raw); }
  forceinline auto compressstore16(void *dest, u16 mask, v256 a) -> void { _mm256_mask_compressstoreu_epi16(dest, mask, a.raw); }
  forceinline auto compressstore16(void *dest, u32 mask, v512 a) -> void { _mm512_mask_compressstoreu_epi16(dest, mask, a.raw); }
  forceinline auto compressstore32(void *dest, u8 mask, v128 a) -> void { _mm_mask_compressstoreu_epi32(dest, mask, a.raw); }
  forceinline auto compressstore32(void *dest, u8 mask, v256 a) -> void { _mm256_mask_compressstoreu_epi32(dest, mask, a.raw); }
  forceinline auto compressstore32(void *dest, u16 mask, v512 a) -> void { _mm512_mask_compressstoreu_epi32(dest, mask, a.raw); }
  forceinline auto compressstore64(void *dest, u8 mask, v128 a) -> void { _mm_mask_compressstoreu_epi64(dest, mask, a.raw); }
  forceinline auto compressstore64(void *dest, u8 mask, v256 a) -> void { _mm256_mask_compressstoreu_epi64(dest, mask, a.raw); }
  forceinline auto compressstore64(void *dest, u8 mask, v512 a) -> void { _mm512_mask_compressstoreu_epi64(dest, mask, a.raw); }

  forceinline auto blend8(u16 mask, v128 a, v128 b) -> v128 { return {_mm_mask_blend_epi8(mask, a.raw, b.raw)}; }
  forceinline auto blend8(u32 mask, v256 a, v256 b) -> v256 { return {_mm256_mask_blend_epi8(mask, a.raw, b.raw)}; }
  forceinline auto blend8(u64 mask, v512 a, v512 b) -> v512 { return {_mm512_mask_blend_epi8(mask, a.raw, b.raw)}; }

  forceinline auto eq8(v128 a, v128 b) -> u16 { return _mm_cmpeq_epu8_mask(a.raw, b.raw); }
  forceinline auto eq8(v256 a, v256 b) -> u32 { return _mm256_cmpeq_epu8_mask(a.raw, b.raw); }
  forceinline auto eq8(v512 a, v512 b) -> u64 { return _mm512_cmpeq_epu8_mask(a.raw, b.raw); }

  template <int index> forceinline auto extract256(v512 a) -> v256 { return {_mm512_extracti64x4_epi64(a.raw, index)}; }

  forceinline auto findset8(v128 haystack, int haystack_len, v128 needles) -> u16 {
    return _mm_extract_epi16(_mm_cmpestrm(haystack.raw, haystack_len, needles.raw, 16, 0), 0);
  }

  forceinline auto gf2p8matmul8(v128 a, v128 b) -> v128 { return {_mm_gf2p8affine_epi64_epi8(a.raw, b.raw, 0)}; }
  forceinline auto gf2p8matmul8(v256 a, v256 b) -> v256 { return {_mm256_gf2p8affine_epi64_epi8(a.raw, b.raw, 0)}; }
  forceinline auto gf2p8matmul8(v512 a, v512 b) -> v512 { return {_mm512_gf2p8affine_epi64_epi8(a.raw, b.raw, 0)}; }

  forceinline auto lanebroadcast8to64(v512 a) -> v512 {
    return vec::gf2p8matmul8(v512::broadcast8(0xFF), vec::gf2p8matmul8(v512::broadcast64(0x0102040810204080), a));
  }

  template <int amount> forceinline auto lanebyteshl(v128 a) -> v128 { return {_mm_slli_si128(a.raw, amount)}; }

  forceinline auto mask8(u16 mask, v128 a) -> v128 { return {_mm_maskz_mov_epi8(mask, a.raw)}; }
  forceinline auto mask8(u32 mask, v256 a) -> v256 { return {_mm256_maskz_mov_epi8(mask, a.raw)}; }
  forceinline auto mask8(u64 mask, v512 a) -> v512 { return {_mm512_maskz_mov_epi8(mask, a.raw)}; }

  forceinline auto mask16(u8 mask, v128 a) -> v128 { return {_mm_maskz_mov_epi16(mask, a.raw)}; }
  forceinline auto mask16(u16 mask, v256 a) -> v256 { return {_mm256_maskz_mov_epi16(mask, a.raw)}; }
  forceinline auto mask16(u32 mask, v512 a) -> v512 { return {_mm512_maskz_mov_epi16(mask, a.raw)}; }

  forceinline auto permute8(v128 index, v128 a) -> v128 { return {_mm_permutexvar_epi8(index.raw, a.raw)}; }
  forceinline auto permute8(v256 index, v256 a) -> v256 { return {_mm256_permutexvar_epi8(index.raw, a.raw)}; }
  forceinline auto permute8(v512 index, v512 a) -> v512 { return {_mm512_permutexvar_epi8(index.raw, a.raw)}; }

  forceinline auto permute8(v512 index, v128 a) -> v512 { return {_mm512_shuffle_epi8(_mm512_broadcast_i32x4(a.raw), index.raw)}; }

  forceinline auto permute8_mz(u16 m, v128 index, v128 a) -> v128 { return {_mm_maskz_permutexvar_epi8(m, index.raw, a.raw)}; }
  forceinline auto permute8_mz(u32 m, v256 index, v256 a) -> v256 { return {_mm256_maskz_permutexvar_epi8(m, index.raw, a.raw)}; }
  forceinline auto permute8_mz(u64 m, v512 index, v512 a) -> v512 { return {_mm512_maskz_permutexvar_epi8(m, index.raw, a.raw)}; }

  forceinline auto permute8(v128 index, v128 a, v128 b) -> v128 { return {_mm_permutex2var_epi8(a.raw, index.raw, b.raw)}; }
  forceinline auto permute8(v256 index, v256 a, v256 b) -> v256 { return {_mm256_permutex2var_epi8(a.raw, index.raw, b.raw)}; }
  forceinline auto permute8(v512 index, v512 a, v512 b) -> v512 { return {_mm512_permutex2var_epi8(a.raw, index.raw, b.raw)}; }

  forceinline auto permute16(v128 index, v128 a) -> v128 { return {_mm_permutexvar_epi16(index.raw, a.raw)}; }
  forceinline auto permute16(v256 index, v256 a) -> v256 { return {_mm256_permutexvar_epi16(index.raw, a.raw)}; }
  forceinline auto permute16(v512 index, v512 a) -> v512 { return {_mm512_permutexvar_epi16(index.raw, a.raw)}; }

  forceinline auto permute16(v128 index, v128 a, v128 b) -> v128 { return {_mm_permutex2var_epi16(a.raw, index.raw, b.raw)}; }
  forceinline auto permute16(v256 index, v256 a, v256 b) -> v256 { return {_mm256_permutex2var_epi16(a.raw, index.raw, b.raw)}; }
  forceinline auto permute16(v512 index, v512 a, v512 b) -> v512 { return {_mm512_permutex2var_epi16(a.raw, index.raw, b.raw)}; }

  forceinline auto permute16_mz(u8 m, v128 index, v128 a, v128 b) -> v128 { return {_mm_maskz_permutex2var_epi16(m, a.raw, index.raw, b.raw)}; }
  forceinline auto permute16_mz(u16 m, v256 index, v256 a, v256 b) -> v256 { return {_mm256_maskz_permutex2var_epi16(m, a.raw, index.raw, b.raw)}; }
  forceinline auto permute16_mz(u32 m, v512 index, v512 a, v512 b) -> v512 { return {_mm512_maskz_permutex2var_epi16(m, a.raw, index.raw, b.raw)}; }

  forceinline auto popcount8(v128 a) -> v128 { return {_mm_popcnt_epi8(a.raw)}; }
  forceinline auto popcount8(v256 a) -> v256 { return {_mm256_popcnt_epi8(a.raw)}; }
  forceinline auto popcount8(v512 a) -> v512 { return {_mm512_popcnt_epi8(a.raw)}; }

  forceinline auto shl16(v128 a, int b) -> v128 { return {_mm_slli_epi16(a.raw, b)}; }
  forceinline auto shl16(v256 a, int b) -> v256 { return {_mm256_slli_epi16(a.raw, b)}; }
  forceinline auto shl16(v512 a, int b) -> v512 { return {_mm512_slli_epi16(a.raw, b)}; }

  forceinline auto shr16(v128 a, int b) -> v128 { return {_mm_srli_epi16(a.raw, b)}; }
  forceinline auto shr16(v256 a, int b) -> v256 { return {_mm256_srli_epi16(a.raw, b)}; }
  forceinline auto shr16(v512 a, int b) -> v512 { return {_mm512_srli_epi16(a.raw, b)}; }

  forceinline auto shl16_mz(u8 m, v128 a, v128 b) -> v128 { return {_mm_maskz_sllv_epi16(m, a.raw, b.raw)}; }
  forceinline auto shl16_mz(u16 m, v256 a, v256 b) -> v256 { return {_mm256_maskz_sllv_epi16(m, a.raw, b.raw)}; }
  forceinline auto shl16_mz(u32 m, v512 a, v512 b) -> v512 { return {_mm512_maskz_sllv_epi16(m, a.raw, b.raw)}; }

  template <u8 shuffle> forceinline auto shuffle128(v512 a) -> v512 { return {_mm512_shuffle_i64x2(a.raw, a.raw, shuffle)}; }

  forceinline auto sub8(v128 a, v128 b) -> v128 { return {_mm_sub_epi8(a.raw, b.raw)}; }
  forceinline auto sub8(v256 a, v256 b) -> v256 { return {_mm256_sub_epi8(a.raw, b.raw)}; }
  forceinline auto sub8(v512 a, v512 b) -> v512 { return {_mm512_sub_epi8(a.raw, b.raw)}; }

  forceinline auto sub8_sat(v128 a, v128 b) -> v128 { return {_mm_subs_epi8(a.raw, b.raw)}; }
  forceinline auto sub8_sat(v256 a, v256 b) -> v256 { return {_mm256_subs_epi8(a.raw, b.raw)}; }
  forceinline auto sub8_sat(v512 a, v512 b) -> v512 { return {_mm512_subs_epi8(a.raw, b.raw)}; }

  forceinline auto test8(v128 a, v128 b) -> u16 { return _mm_test_epi8_mask(a.raw, b.raw); }
  forceinline auto test8(v256 a, v256 b) -> u32 { return _mm256_test_epi8_mask(a.raw, b.raw); }
  forceinline auto test8(v512 a, v512 b) -> u64 { return _mm512_test_epi8_mask(a.raw, b.raw); }
  forceinline auto test16(v128 a, v128 b) -> u8 { return _mm_test_epi16_mask(a.raw, b.raw); }
  forceinline auto test16(v256 a, v256 b) -> u16 { return _mm256_test_epi16_mask(a.raw, b.raw); }
  forceinline auto test16(v512 a, v512 b) -> u32 { return _mm512_test_epi16_mask(a.raw, b.raw); }

  forceinline auto testn8(v128 a, v128 b) -> u16 { return _mm_testn_epi8_mask(a.raw, b.raw); }
  forceinline auto testn8(v256 a, v256 b) -> u32 { return _mm256_testn_epi8_mask(a.raw, b.raw); }
  forceinline auto testn8(v512 a, v512 b) -> u64 { return _mm512_testn_epi8_mask(a.raw, b.raw); }
  forceinline auto testn16(v128 a, v128 b) -> u16 { return _mm_testn_epi16_mask(a.raw, b.raw); }
  forceinline auto testn16(v256 a, v256 b) -> u32 { return _mm256_testn_epi16_mask(a.raw, b.raw); }
  forceinline auto testn16(v512 a, v512 b) -> u64 { return _mm512_testn_epi16_mask(a.raw, b.raw); }

  forceinline auto truncate64to16(v128 a) -> v128 { return {_mm_cvtepi64_epi16(a.raw)}; }
  forceinline auto truncate64to16(v256 a) -> v128 { return {_mm256_cvtepi64_epi16(a.raw)}; }
  forceinline auto truncate64to16(v512 a) -> v128 { return {_mm512_cvtepi64_epi16(a.raw)}; }

  forceinline auto zext8to16_lo(v128 a) -> v128 { return {_mm_cvtepu8_epi16(a.raw)}; }
  forceinline auto zext8to16(v128 a) -> v256 { return {_mm256_cvtepu8_epi16(a.raw)}; }
  forceinline auto zext8to16(v256 a) -> v512 { return {_mm512_cvtepu8_epi16(a.raw)}; }

} // namespace rose::vec
