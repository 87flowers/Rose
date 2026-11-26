#pragma once

#include "lps2/arch.hpp"
#include "lps2/stdint.hpp"

#include <array>
#include <bit>
#include <climits>

#if LPS2_ARCH_X86_64
#include <immintrin.h>
#endif

#if LPS2_ARCH_AARCH64
#include <arm_neon.h>
#endif

namespace lps2::detail {

  template<class T, usize N>
  struct vector_arch_type;

  template<class T, usize N>
  using vector_arch_type_t = typename vector_arch_type<T, N>::type;

#if LPS2_ARCH_X86_64
  template<usize bitsize>
  struct vector_arch_type_helper;

  template<>
  struct vector_arch_type_helper<0> {
    static inline constexpr bool is_doubled = false;
    using type = void;
  };

  template<>
  struct vector_arch_type_helper<128> {
    static inline constexpr bool is_doubled = false;
    using type = __m128i;
  };

#if LPS2_ARCH_AVX
  template<>
  struct vector_arch_type_helper<256> {
    static inline constexpr bool is_doubled = false;
    using type = __m256i;
  };
#endif

#if LPS2_ARCH_AVX512BASE
  template<>
  struct vector_arch_type_helper<512> {
    static inline constexpr bool is_doubled = false;
    using type = __m512i;
  };
#endif

  template<usize bitsize>
  struct vector_arch_type_helper {
    static_assert(std::has_single_bit(bitsize) || bitsize == 0);

    static inline constexpr bool is_doubled = true;
    using type = std::array<typename vector_arch_type_helper<bitsize / 2>::type, 2>;
  };

  template<class T, usize N>
  struct array_wrapper {
    using type = std::array<T, N>;
  };

  template<class T, usize N>
  struct vector_arch_type {
    static inline constexpr usize bitsize = sizeof(T) * N * CHAR_BIT;
    static inline constexpr bool is_doubled = bitsize >= 128 && vector_arch_type_helper<bitsize>::is_doubled;
    using type = std::conditional_t<bitsize >= 128, vector_arch_type_helper<bitsize>, array_wrapper<T, N>>::type;
  };
#endif

#if LPS2_ARCH_AARCH64
  template<class T, usize N>
  struct vector_arch_type {
    static_assert(std::has_single_bit(N) || N == 0);

    static inline constexpr usize bitsize = sizeof(T) * N * CHAR_BIT;
    static inline constexpr bool is_doubled = bitsize >= 128;
    using type = std::conditional_t<bitsize >= 128, std::array<typename vector_arch_type<T, N / 2>::type, 2>, std::array<T, N>>;
  };

#define LPS2_DECL_VECTOR_TYPES(TN, N)                                                                                                                \
  template<>                                                                                                                                         \
  struct vector_arch_type<u##TN, N> {                                                                                                                \
    static inline constexpr bool is_doubled = false;                                                                                                 \
    using type = uint##TN##x##N##_t;                                                                                                                 \
  };                                                                                                                                                 \
  template<>                                                                                                                                         \
  struct vector_arch_type<i##TN, N> {                                                                                                                \
    static inline constexpr bool is_doubled = false;                                                                                                 \
    using type = int##TN##x##N##_t;                                                                                                                  \
  };

  LPS2_DECL_VECTOR_TYPES(8, 8)
  LPS2_DECL_VECTOR_TYPES(8, 16)
  LPS2_DECL_VECTOR_TYPES(16, 4)
  LPS2_DECL_VECTOR_TYPES(16, 8)
  LPS2_DECL_VECTOR_TYPES(32, 2)
  LPS2_DECL_VECTOR_TYPES(32, 4)
  LPS2_DECL_VECTOR_TYPES(64, 1)
  LPS2_DECL_VECTOR_TYPES(64, 2)

#undef LPS2_DECL_VECTOR_TYPES

#endif

}  // namespace lps2::detail
