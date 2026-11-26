#pragma once

#include "lps2/detail/signed_double_element_size.hpp"
#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N, class U>
    requires(sizeof(T) == sizeof(U))
  LPS2_INLINE constexpr auto pair_dot(const vector<T, N>& first, const vector<U, N>& second) {
    using S = detail::signed_double_element_size_t<T>;
    std::array<S, N / 2> result_raw;
    std::array<T, N> first_raw = first.to_array();
    std::array<U, N> second_raw = second.to_array();
    for (usize i = 0; i < N; i += 2) {
      result_raw[i / 2] =
        static_cast<S>(first_raw[i]) * static_cast<S>(second_raw[i]) + static_cast<S>(first_raw[i + 1]) * static_cast<S>(second_raw[i + 1]);
    }
    return std::bit_cast<vector<S, N / 2>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  template<class U>
    requires(sizeof(T) == sizeof(U))
  LPS2_INLINE constexpr auto vector<T, N>::pair_dot(const vector<U, N>& second) const {
    using S = detail::signed_double_element_size_t<T>;
    using Result = vector<S, N / 2>;

#if LPS2_ARCH_SSSE3
    if constexpr (bitsize == 128 && std::is_same_v<T, u8> && std::is_same_v<U, i8>) {
      return Result { _mm_maddubs_epi16(raw, second.raw) };
    }
#endif

#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128 && std::is_same_v<T, i16> && std::is_same_v<U, i16>) {
      return Result { _mm_madd_epi16(raw, second.raw) };
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256 && std::is_same_v<T, u8> && std::is_same_v<U, i8>) {
      return Result { _mm256_maddubs_epi16(raw, second.raw) };
    }
    if constexpr (bitsize == 256 && std::is_same_v<T, i16> && std::is_same_v<U, i16>) {
      return Result { _mm256_madd_epi16(raw, second.raw) };
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512 && std::is_same_v<T, u8> && std::is_same_v<U, i8>) {
      return Result { _mm512_maddubs_epi16(raw, second.raw) };
    }
    if constexpr (bitsize == 512 && std::is_same_v<T, i16> && std::is_same_v<U, i16>) {
      return Result { _mm512_madd_epi16(raw, second.raw) };
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.pair_dot(second.raw[0]).raw;
      result.raw[1] = Half { raw[1] }.pair_dot(second.raw[1]).raw;
      return result;
    }

    return generic::vector_ops::pair_dot(*this, second);
  }
}  // namespace lps2
