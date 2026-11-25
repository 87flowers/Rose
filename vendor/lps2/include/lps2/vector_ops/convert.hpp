#pragma once

#include "lps2/detail/vector_clamped_size.hpp"
#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <array>
#include <bit>
#include <type_traits>

namespace lps2::generic::vector_ops {
  template<class U, class T, usize N>
  constexpr vector<U, detail::clamped_size<U, N>> convert(const vector<T, N>& self) {
    std::array<T, N> self_raw = self.to_array();
    std::array<U, detail::clamped_size<U, N>> result_raw {};
    result_raw.fill(0);
    for (usize i = 0; i < N; i++) {
      result_raw[i] = static_cast<U>(self_raw[i]);
    }
    return std::bit_cast<vector<U, detail::clamped_size<U, N>>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2::detail {
  template<class T, class U>
  inline constexpr bool should_sign_extend = std::is_signed_v<T> && sizeof(U) > sizeof(T);

  template<class T, class U, usize N, usize dfd, usize T_size, usize U_size, bool sign_extend>
  struct convert_simd;

#define LPS2_DEFINE_CONVERT(bitsize, T_, U_, ...)                                                                                                    \
  template<class T, class U, usize N>                                                                                                                \
  struct convert_simd<T, U, N, bitsize, sizeof(T_), sizeof(U_), should_sign_extend<T_, U_>> {                                                        \
    using Result = detail::vector_clamped_size<U, N>;                                                                                                \
    LPS2_INLINE constexpr Result operator()(vector<T, N> v) __VA_ARGS__                                                                              \
  };

  // Convert to smaller

#if LPS2_ARCH_SSE4_2 && !LPS2_ARCH_AVX512BASE
  LPS2_DEFINE_CONVERT(128, u16, u8, { return Result { _mm_packus_epi16(_mm_and_si128(v.raw, _mm_set1_epi16(0xFF)), _mm_setzero_si128()) }; })
  LPS2_DEFINE_CONVERT(128, u32, u16, { return Result { _mm_packus_epi32(_mm_and_si128(v.raw, _mm_set1_epi32(0xFFFF)), _mm_setzero_si128()) }; })
  LPS2_DEFINE_CONVERT(128, u64, u32, { return Result { (__m128i)_mm_shuffle_ps((__m128)v.raw, _mm_setzero_ps(), 0b1000) }; })

  LPS2_DEFINE_CONVERT(128, u32, u8, {
    return Result { _mm_packus_epi16(_mm_packus_epi32(_mm_and_si128(v.raw, _mm_set1_epi32(0xFF)), _mm_setzero_si128()), _mm_setzero_si128()) };
  })
  LPS2_DEFINE_CONVERT(128, u64, u16, {
    return Result { _mm_packus_epi32((__m128i)_mm_shuffle_ps((__m128)_mm_and_si128(v.raw, _mm_set1_epi32(0xFFFF)), _mm_setzero_ps(), 0b1000),
                                     _mm_setzero_si128()) };
  })
  LPS2_DEFINE_CONVERT(128, u64, u8, {
    return Result { _mm_packus_epi16(
      _mm_packus_epi32((__m128i)_mm_shuffle_ps((__m128)_mm_and_si128(v.raw, _mm_set1_epi32(0xFF)), _mm_setzero_ps(), 0b1000), _mm_setzero_si128()),
      _mm_setzero_si128()) };
  })

  LPS2_DEFINE_CONVERT(256, u16, u8, {
    return Result { _mm_packus_epi16(_mm_and_si128(v.template extract_aligned<__m128i, 0>(), _mm_set1_epi16(0xFF)),
                                     _mm_and_si128(v.template extract_aligned<__m128i, 1>(), _mm_set1_epi16(0xFF))) };
  })
  LPS2_DEFINE_CONVERT(256, u32, u16, {
    return Result { _mm_packus_epi32(_mm_and_si128(v.template extract_aligned<__m128i, 0>(), _mm_set1_epi32(0xFFFF)),
                                     _mm_and_si128(v.template extract_aligned<__m128i, 1>(), _mm_set1_epi32(0xFFFF))) };
  })
  LPS2_DEFINE_CONVERT(256, u64, u32, {
    return Result { (__m128i)_mm_shuffle_ps((v.template extract_aligned<__m128, 0>()), (v.template extract_aligned<__m128, 1>()), 0b10001000) };
  })

  LPS2_DEFINE_CONVERT(256, u32, u8, {
    return Result { _mm_packus_epi16(_mm_packus_epi32(_mm_and_si128(v.template extract_aligned<__m128i, 0>(), _mm_set1_epi32(0xFF)),
                                                      _mm_and_si128(v.template extract_aligned<__m128i, 1>(), _mm_set1_epi32(0xFF))),
                                     _mm_setzero_si128()) };
  })
  LPS2_DEFINE_CONVERT(256, u64, u16, {
    return Result { _mm_packus_epi32(
      _mm_and_si128((__m128i)_mm_shuffle_ps((v.template extract_aligned<__m128, 0>()), (v.template extract_aligned<__m128, 1>()), 0b10001000),
                    _mm_set1_epi32(0xFFFF)),
      _mm_setzero_si128()) };
  })
  LPS2_DEFINE_CONVERT(256, u64, u8, {
    return Result { _mm_packus_epi16(_mm_packus_epi32(_mm_and_si128((__m128i)_mm_shuffle_ps((v.template extract_aligned<__m128, 0>()),
                                                                                            (v.template extract_aligned<__m128, 1>()), 0b10001000),
                                                                    _mm_set1_epi32(0xFF)),
                                                      _mm_setzero_si128()),
                                     _mm_setzero_si128()) };
  })
#endif

#if LPS2_ARCH_AVX512BASE
  LPS2_DEFINE_CONVERT(128, u16, u8, { return Result { _mm_cvtepi16_epi8(v.raw) }; });
  LPS2_DEFINE_CONVERT(128, u32, u16, { return Result { _mm_cvtepi32_epi16(v.raw) }; });
  LPS2_DEFINE_CONVERT(128, u64, u32, { return Result { _mm_cvtepi64_epi32(v.raw) }; });
  LPS2_DEFINE_CONVERT(128, u32, u8, { return Result { _mm_cvtepi32_epi8(v.raw) }; });
  LPS2_DEFINE_CONVERT(128, u64, u16, { return Result { _mm_cvtepi64_epi16(v.raw) }; });
  LPS2_DEFINE_CONVERT(128, u64, u8, { return Result { _mm_cvtepi64_epi8(v.raw) }; });
  LPS2_DEFINE_CONVERT(256, u16, u8, { return Result { _mm256_cvtepi16_epi8(v.raw) }; });
  LPS2_DEFINE_CONVERT(256, u32, u16, { return Result { _mm256_cvtepi32_epi16(v.raw) }; });
  LPS2_DEFINE_CONVERT(256, u64, u32, { return Result { _mm256_cvtepi64_epi32(v.raw) }; });
  LPS2_DEFINE_CONVERT(256, u32, u8, { return Result { _mm256_cvtepi32_epi8(v.raw) }; });
  LPS2_DEFINE_CONVERT(256, u64, u16, { return Result { _mm256_cvtepi64_epi16(v.raw) }; });
  LPS2_DEFINE_CONVERT(256, u64, u8, { return Result { _mm256_cvtepi64_epi8(v.raw) }; });
  LPS2_DEFINE_CONVERT(512, u16, u8, { return Result { _mm512_cvtepi16_epi8(v.raw) }; });
  LPS2_DEFINE_CONVERT(512, u32, u16, { return Result { _mm512_cvtepi32_epi16(v.raw) }; });
  LPS2_DEFINE_CONVERT(512, u64, u32, { return Result { _mm512_cvtepi64_epi32(v.raw) }; });
  LPS2_DEFINE_CONVERT(512, u32, u8, { return Result { _mm512_cvtepi32_epi8(v.raw) }; });
  LPS2_DEFINE_CONVERT(512, u64, u16, { return Result { _mm512_cvtepi64_epi16(v.raw) }; });
  LPS2_DEFINE_CONVERT(512, u64, u8, { return Result { _mm512_cvtepi64_epi8(v.raw) }; });
#endif

  // Convert to bigger

#if LPS2_ARCH_AVX2
  LPS2_DEFINE_CONVERT(128, u8, u16, { return Result { _mm256_cvtepu8_epi16(v.raw) }; })
  LPS2_DEFINE_CONVERT(128, i8, i16, { return Result { _mm256_cvtepi8_epi16(v.raw) }; })
  LPS2_DEFINE_CONVERT(128, u16, u32, { return Result { _mm256_cvtepu16_epi32(v.raw) }; })
  LPS2_DEFINE_CONVERT(128, i16, i32, { return Result { _mm256_cvtepi16_epi32(v.raw) }; })
  LPS2_DEFINE_CONVERT(128, u32, u64, { return Result { _mm256_cvtepu32_epi64(v.raw) }; })
  LPS2_DEFINE_CONVERT(128, i32, i64, { return Result { _mm256_cvtepi32_epi64(v.raw) }; })
#endif

#if LPS2_ARCH_AVX512BASE
  LPS2_DEFINE_CONVERT(128, u8, u32, { return Result { _mm512_cvtepu8_epi32(v.raw) }; })
  LPS2_DEFINE_CONVERT(128, i8, i32, { return Result { _mm512_cvtepi8_epi32(v.raw) }; })
  LPS2_DEFINE_CONVERT(128, u16, u64, { return Result { _mm512_cvtepu16_epi64(v.raw) }; })
  LPS2_DEFINE_CONVERT(128, i16, i64, { return Result { _mm512_cvtepi16_epi64(v.raw) }; })
#endif

#if LPS2_ARCH_AVX512BASE
  LPS2_DEFINE_CONVERT(256, u8, u16, { return Result { _mm512_cvtepu8_epi16(v.raw) }; })
  LPS2_DEFINE_CONVERT(256, i8, i16, { return Result { _mm512_cvtepi8_epi16(v.raw) }; })
  LPS2_DEFINE_CONVERT(256, u16, u32, { return Result { _mm512_cvtepu16_epi32(v.raw) }; })
  LPS2_DEFINE_CONVERT(256, i16, i32, { return Result { _mm512_cvtepi16_epi32(v.raw) }; })
  LPS2_DEFINE_CONVERT(256, u32, u64, { return Result { _mm512_cvtepu32_epi64(v.raw) }; })
  LPS2_DEFINE_CONVERT(256, i32, i64, { return Result { _mm512_cvtepi32_epi64(v.raw) }; })
#endif

#if LPS2_ARCH_AVX512BASE
  LPS2_DEFINE_CONVERT(512, u8, u16, {
    return std::bit_cast<Result>(
      std::array { _mm512_cvtepu8_epi16(_mm512_castsi512_si256(v.raw)), _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(v.raw, 1)) });
  })
  LPS2_DEFINE_CONVERT(512, i8, i16, {
    return std::bit_cast<Result>(
      std::array { _mm512_cvtepi8_epi16(_mm512_castsi512_si256(v.raw)), _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(v.raw, 1)) });
  })
  LPS2_DEFINE_CONVERT(512, u16, u32, {
    return std::bit_cast<Result>(
      std::array { _mm512_cvtepu16_epi32(_mm512_castsi512_si256(v.raw)), _mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64(v.raw, 1)) });
  })
  LPS2_DEFINE_CONVERT(512, i16, i32, {
    return std::bit_cast<Result>(
      std::array { _mm512_cvtepi16_epi32(_mm512_castsi512_si256(v.raw)), _mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(v.raw, 1)) });
  })
  LPS2_DEFINE_CONVERT(512, u32, u64, {
    return std::bit_cast<Result>(
      std::array { _mm512_cvtepu32_epi64(_mm512_castsi512_si256(v.raw)), _mm512_cvtepu32_epi64(_mm512_extracti64x4_epi64(v.raw, 1)) });
  })
  LPS2_DEFINE_CONVERT(512, i32, i64, {
    return std::bit_cast<Result>(
      std::array { _mm512_cvtepi32_epi64(_mm512_castsi512_si256(v.raw)), _mm512_cvtepi32_epi64(_mm512_extracti64x4_epi64(v.raw, 1)) });
  })
#endif

#undef LPS2_DEFINE_CONVERT

}  // namespace lps2::detail

namespace lps2 {
  template<class T, usize N>
  template<class U>
  LPS2_INLINE constexpr detail::vector_clamped_size<U, N> vector<T, N>::convert() const {
    if constexpr (std::is_same_v<T, U>) {
      return *this;
    }

    if constexpr (sizeof(T) == sizeof(U)) {
      return std::bit_cast<detail::vector_clamped_size<U, N>>(*this);
    }

    if constexpr (requires { detail::convert_simd<T, U, N, bitsize, sizeof(T), sizeof(U), detail::should_sign_extend<T, U>> {}(*this); }) {
      return detail::convert_simd<T, U, N, bitsize, sizeof(T), sizeof(U), detail::should_sign_extend<T, U>> {}(*this);
    }

    return generic::vector_ops::convert<U, T, N>(*this);
  }
}  // namespace lps2
