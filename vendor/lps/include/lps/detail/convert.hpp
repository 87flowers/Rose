#pragma once

#include "lps/detail/vector_clamped_size.hpp"
#include "lps/generic/vector.hpp"
#include "lps/stdint.hpp"

#include <bit>
#include <type_traits>

namespace lps::detail {

  template<class T>
  constexpr usize vector_bitsize() {
    return CHAR_BIT * sizeof(T);
  }

  template<class T, class U>
  inline constexpr bool should_sign_extend = std::is_signed_v<T> && sizeof(U) > sizeof(T);

  template<class Env, class T, class U, usize N, usize dfd, usize T_size, usize U_size, bool sign_extend>
  struct convert_simd;

  template<class Env, class T, class U, usize N>
  LPS_INLINE constexpr vector_clamped_size<Env, U, N> convert(typename Env::template vector<T, N> v) {
    if constexpr (sizeof(T) == sizeof(U)) {
      return v;
    } else if constexpr (requires {
                           convert_simd<Env, T, U, N, vector_bitsize<decltype(v)>(), sizeof(T), sizeof(U), should_sign_extend<T, U>> {}(v);
                         }) {
      return convert_simd<Env, T, U, N, vector_bitsize<decltype(v)>(), sizeof(T), sizeof(U), should_sign_extend<T, U>> {}(v);
    } else {
      generic::vector<U, std::max(N, 16 / sizeof(U))> result {};
      for (usize i = 0; i < N; i++) {
        result.raw[i] = static_cast<U>(v.read(i));
      }
      return std::bit_cast<vector_clamped_size<Env, U, N>>(result);
    }
  }

#define LPS_DEFINE_CONVERT(bitsize, T_, U_, ...)                                                                                                     \
  template<class Env, class T, class U, usize N>                                                                                                     \
  struct convert_simd<Env, T, U, N, bitsize, sizeof(T_), sizeof(U_), should_sign_extend<T_, U_>> {                                                   \
    LPS_INLINE constexpr vector_clamped_size<Env, U, N> operator()(typename Env::template vector<T, N> v) __VA_ARGS__                                \
  };

  // Convert to smaller

#if LPS_SSE4_2 || LPS_AVX2
  LPS_DEFINE_CONVERT(128, u16, u8, { return { _mm_packus_epi16(_mm_and_si128(v.raw, _mm_set1_epi16(0xFF)), _mm_setzero_si128()) }; })
  LPS_DEFINE_CONVERT(128, u32, u16, { return { _mm_packus_epi32(_mm_and_si128(v.raw, _mm_set1_epi32(0xFFFF)), _mm_setzero_si128()) }; })
  LPS_DEFINE_CONVERT(128, u64, u32, { return { (__m128i)_mm_shuffle_ps((__m128)v.raw, _mm_setzero_ps(), 0b1000) }; })

  LPS_DEFINE_CONVERT(128, u32, u8, {
    return { _mm_packus_epi16(_mm_packus_epi32(_mm_and_si128(v.raw, _mm_set1_epi32(0xFF)), _mm_setzero_si128()), _mm_setzero_si128()) };
  })
  LPS_DEFINE_CONVERT(128, u64, u16, {
    return { _mm_packus_epi32((__m128i)_mm_shuffle_ps((__m128)_mm_and_si128(v.raw, _mm_set1_epi32(0xFFFF)), _mm_setzero_ps(), 0b1000),
                              _mm_setzero_si128()) };
  })
  LPS_DEFINE_CONVERT(128, u64, u8, {
    return { _mm_packus_epi16(
      _mm_packus_epi32((__m128i)_mm_shuffle_ps((__m128)_mm_and_si128(v.raw, _mm_set1_epi32(0xFF)), _mm_setzero_ps(), 0b1000), _mm_setzero_si128()),
      _mm_setzero_si128()) };
  })

  LPS_DEFINE_CONVERT(256, u16, u8, {
    return { _mm_packus_epi16(_mm_and_si128(v.template extract_aligned<__m128i, 0>(), _mm_set1_epi16(0xFF)),
                              _mm_and_si128(v.template extract_aligned<__m128i, 1>(), _mm_set1_epi16(0xFF))) };
  })
  LPS_DEFINE_CONVERT(256, u32, u16, {
    return { _mm_packus_epi32(_mm_and_si128(v.template extract_aligned<__m128i, 0>(), _mm_set1_epi32(0xFFFF)),
                              _mm_and_si128(v.template extract_aligned<__m128i, 1>(), _mm_set1_epi32(0xFFFF))) };
  })
  LPS_DEFINE_CONVERT(256, u64, u32, {
    return { (__m128i)_mm_shuffle_ps(v.template extract_aligned<__m128, 0>(), v.template extract_aligned<__m128, 1>(), 0b10001000) };
  })

  LPS_DEFINE_CONVERT(256, u32, u8, {
    return { _mm_packus_epi16(_mm_packus_epi32(_mm_and_si128(v.template extract_aligned<__m128i, 0>(), _mm_set1_epi32(0xFF)),
                                               _mm_and_si128(v.template extract_aligned<__m128i, 1>(), _mm_set1_epi32(0xFF))),
                              _mm_setzero_si128()) };
  })
  LPS_DEFINE_CONVERT(256, u64, u16, {
    return { _mm_packus_epi32(
      _mm_and_si128((__m128i)_mm_shuffle_ps(v.template extract_aligned<__m128, 0>(), v.template extract_aligned<__m128, 1>(), 0b10001000),
                    _mm_set1_epi32(0xFFFF)),
      _mm_setzero_si128()) };
  })
  LPS_DEFINE_CONVERT(256, u64, u8, {
    return { _mm_packus_epi16(_mm_packus_epi32(_mm_and_si128((__m128i)_mm_shuffle_ps(v.template extract_aligned<__m128, 0>(),
                                                                                     v.template extract_aligned<__m128, 1>(), 0b10001000),
                                                             _mm_set1_epi32(0xFF)),
                                               _mm_setzero_si128()),
                              _mm_setzero_si128()) };
  })
#endif

#if LPS_AVX512
  LPS_DEFINE_CONVERT(128, u16, u8, { return { _mm_cvtepi16_epi8(v.raw) }; });
  LPS_DEFINE_CONVERT(128, u32, u16, { return { _mm_cvtepi32_epi16(v.raw) }; });
  LPS_DEFINE_CONVERT(128, u64, u32, { return { _mm_cvtepi64_epi32(v.raw) }; });
  LPS_DEFINE_CONVERT(128, u32, u8, { return { _mm_cvtepi32_epi8(v.raw) }; });
  LPS_DEFINE_CONVERT(128, u64, u16, { return { _mm_cvtepi64_epi16(v.raw) }; });
  LPS_DEFINE_CONVERT(128, u64, u8, { return { _mm_cvtepi64_epi8(v.raw) }; });
  LPS_DEFINE_CONVERT(256, u16, u8, { return { _mm256_cvtepi16_epi8(v.raw) }; });
  LPS_DEFINE_CONVERT(256, u32, u16, { return { _mm256_cvtepi32_epi16(v.raw) }; });
  LPS_DEFINE_CONVERT(256, u64, u32, { return { _mm256_cvtepi64_epi32(v.raw) }; });
  LPS_DEFINE_CONVERT(256, u32, u8, { return { _mm256_cvtepi32_epi8(v.raw) }; });
  LPS_DEFINE_CONVERT(256, u64, u16, { return { _mm256_cvtepi64_epi16(v.raw) }; });
  LPS_DEFINE_CONVERT(256, u64, u8, { return { _mm256_cvtepi64_epi8(v.raw) }; });
  LPS_DEFINE_CONVERT(512, u16, u8, { return { _mm512_cvtepi16_epi8(v.raw) }; });
  LPS_DEFINE_CONVERT(512, u32, u16, { return { _mm512_cvtepi32_epi16(v.raw) }; });
  LPS_DEFINE_CONVERT(512, u64, u32, { return { _mm512_cvtepi64_epi32(v.raw) }; });
  LPS_DEFINE_CONVERT(512, u32, u8, { return { _mm512_cvtepi32_epi8(v.raw) }; });
  LPS_DEFINE_CONVERT(512, u64, u16, { return { _mm512_cvtepi64_epi16(v.raw) }; });
  LPS_DEFINE_CONVERT(512, u64, u8, { return { _mm512_cvtepi64_epi8(v.raw) }; });
#endif

  // Convert to bigger

#if LPS_AVX2 || LPS_AVX512
  LPS_DEFINE_CONVERT(128, u8, u16, { return { _mm256_cvtepu8_epi16(v.raw) }; })
  LPS_DEFINE_CONVERT(128, i8, i16, { return { _mm256_cvtepi8_epi16(v.raw) }; })
  LPS_DEFINE_CONVERT(128, u16, u32, { return { _mm256_cvtepu16_epi32(v.raw) }; })
  LPS_DEFINE_CONVERT(128, i16, i32, { return { _mm256_cvtepi16_epi32(v.raw) }; })
  LPS_DEFINE_CONVERT(128, u32, u64, { return { _mm256_cvtepu32_epi64(v.raw) }; })
  LPS_DEFINE_CONVERT(128, i32, i64, { return { _mm256_cvtepi32_epi64(v.raw) }; })
#endif

#if LPS_AVX512
  LPS_DEFINE_CONVERT(256, u8, u16, { return { _mm512_cvtepu8_epi16(v.raw) }; })
  LPS_DEFINE_CONVERT(256, i8, i16, { return { _mm512_cvtepi8_epi16(v.raw) }; })
  LPS_DEFINE_CONVERT(256, u16, u32, { return { _mm512_cvtepu16_epi32(v.raw) }; })
  LPS_DEFINE_CONVERT(256, i16, i32, { return { _mm512_cvtepi16_epi32(v.raw) }; })
  LPS_DEFINE_CONVERT(256, u32, u64, { return { _mm512_cvtepu32_epi64(v.raw) }; })
  LPS_DEFINE_CONVERT(256, i32, i64, { return { _mm512_cvtepi32_epi64(v.raw) }; })
#endif

#if LPS_AVX512
  LPS_DEFINE_CONVERT(128, u8, u32, { return { _mm512_cvtepu8_epi32(v.raw) }; })
  LPS_DEFINE_CONVERT(128, i8, i32, { return { _mm512_cvtepi8_epi32(v.raw) }; })
  LPS_DEFINE_CONVERT(128, u16, u64, { return { _mm512_cvtepu16_epi64(v.raw) }; })
  LPS_DEFINE_CONVERT(128, i16, i64, { return { _mm512_cvtepi16_epi64(v.raw) }; })
#endif

}  // namespace lps::detail
