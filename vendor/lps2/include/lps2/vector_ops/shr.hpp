#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>
#include <bit>

namespace lps2::generic::vector_ops {
  template<usize shift_amount, class T, usize N>
  LPS2_INLINE constexpr vector<T, N> shr(const vector<T, N>& self) {
    std::array<T, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = self_raw[i] >> shift_amount;
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  template<usize shift_amount>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::shr() const {
    if constexpr (std::is_signed_v<T>) {
#if LPS2_ARCH_X86_64
      if constexpr (bitsize == 128) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          // TODO: GFNI
          const __m128i a = _mm_srai_epi16(raw, shift_amount);
          const __m128i b = _mm_srai_epi16(_mm_slli_epi16(raw, 8), shift_amount + 8);
#if LPS2_ARCH_AVX512BASE
          return vector { _mm_mask_mov_epi8(a, 0x5555, b) };
#elif LPS2_ARCH_SSE4_1
          return vector { _mm_blendv_epi8(a, b, _mm_set1_epi16(0x00FF)) };
#else
          return vector { _mm_or_si128(_mm_and_si128(a, _mm_set1_epi16(0xFF00)), _mm_and_si128(b, _mm_set1_epi16(0x00FF))) };
#endif
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vector { _mm_srai_epi16(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vector { _mm_srai_epi32(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
#if LPS2_ARCH_AVX512BASE
          return vector { _mm_srai_epi64(raw, shift_amount) };
#else
          const __m128i a = _mm_srli_epi64(raw, shift_amount);
          const __m128i b = _mm_sub_epi64(_mm_setzero_si128(), _mm_srli_epi64(_mm_and_si128(raw, _mm_set1_epi64x(0x8000000000000000)), shift_amount));
          return vector { _mm_or_si128(a, b) };
#endif
        }
      }
#endif
#if LPS2_ARCH_AVX2
      if constexpr (bitsize == 256) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          // TODO: GFNI
          const __m256i a = _mm256_srai_epi16(raw, shift_amount);
          const __m256i b = _mm256_srai_epi16(_mm256_slli_epi16(raw, 8), shift_amount + 8);
#if LPS2_ARCH_AVX512BASE
          return vector { _mm256_mask_mov_epi8(a, 0x55555555, b) };
#else
          return vector { _mm256_blendv_epi8(a, b, _mm256_set1_epi16(0x00FF)) };
#endif
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vector { _mm256_srai_epi16(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vector { _mm256_srai_epi32(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
#if LPS2_ARCH_AVX512BASE
          return vector { _mm256_srai_epi64(raw, shift_amount) };
#else
          const __m256i a = _mm256_srli_epi64(raw, shift_amount);
          const __m256i b =
            _mm256_sub_epi64(_mm256_setzero_si256(), _mm256_srli_epi64(_mm256_and_si256(raw, _mm256_set1_epi64x(0x8000000000000000)), shift_amount));
          return vector { _mm256_or_si256(a, b) };
#endif
        }
      }
#endif
#if LPS2_ARCH_AVX512BASE
      if constexpr (bitsize == 512) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          // TODO: GFNI
          const __m512i a = _mm512_srai_epi16(raw, shift_amount);
          const __m512i b = _mm512_srai_epi16(_mm512_slli_epi16(raw, 8), shift_amount + 8);
          return vector { _mm512_mask_mov_epi8(a, 0x5555555555555555, b) };
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vector { _mm512_srai_epi16(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vector { _mm512_srai_epi32(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
          return vector { _mm512_srai_epi64(raw, shift_amount) };
        }
      }
#endif
    }

    if constexpr (!std::is_signed_v<T>) {
#if LPS2_ARCH_X86_64
      if constexpr (bitsize == 128) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          constexpr u8 mask = static_cast<u8>(0xFF >> shift_amount);
          return vector { _mm_srli_epi16(raw, shift_amount) } & vector::splat(mask);
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vector { _mm_srli_epi16(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vector { _mm_srli_epi32(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
          return vector { _mm_srli_epi64(raw, shift_amount) };
        }
      }
#endif
#if LPS2_ARCH_AVX2
      if constexpr (bitsize == 256) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          constexpr u8 mask = static_cast<u8>(0xFF >> shift_amount);
          return vector { _mm256_srli_epi16(raw, shift_amount) } & vector::splat(mask);
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vector { _mm256_srli_epi16(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vector { _mm256_srli_epi32(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
          return vector { _mm256_srli_epi64(raw, shift_amount) };
        }
      }
#endif
#if LPS2_ARCH_AVX512BASE
      if constexpr (bitsize == 512) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          constexpr u8 mask = static_cast<u8>(0xFF >> shift_amount);
          return vector { _mm512_srli_epi16(raw, shift_amount) } & vector::splat(mask);
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vector { _mm512_srli_epi16(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vector { _mm512_srli_epi32(raw, shift_amount) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
          return vector { _mm512_srli_epi64(raw, shift_amount) };
        }
      }
#endif
    }

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.template shr<shift_amount>().raw;
      result.raw[1] = Half { raw[1] }.template shr<shift_amount>().raw;
      return result;
    }

    return generic::vector_ops::shr<shift_amount>(*this);
  }
}  // namespace lps2
