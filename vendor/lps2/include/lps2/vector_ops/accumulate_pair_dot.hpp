#pragma once

#include "lps2/detail/signed_double_element_size.hpp"
#include "lps2/vector.hpp"

#include <array>

namespace lps2 {
  template<class T, usize N>
  template<class V1, class V2>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::accumulate_pair_dot(const V1& first, const V2& second) const {
    using U1 = V1::element_type;
    using U2 = V2::element_type;
    static_assert(sizeof(U1) == sizeof(U2) && sizeof(vector) == sizeof(V1) && sizeof(vector) == sizeof(V2));
    static_assert(std::is_same_v<T, detail::signed_double_element_size_t<U1>> && std::is_same_v<T, detail::signed_double_element_size_t<U2>>);

#if LPS2_ARCH_AVX_VNNI || LPS2_ARCH_AVX512VNNI
    if constexpr (bitsize == 128 && std::is_same_v<U1, i16> && std::is_same_v<U2, i16>) {
      return vector { _mm_dpwssd_epi32(raw, first.raw, second.raw) };
    }
    if constexpr (bitsize == 256 && std::is_same_v<U1, i16> && std::is_same_v<U2, i16>) {
      return vector { _mm256_dpwssd_epi32(raw, first.raw, second.raw) };
    }
#endif
#if LPS2_ARCH_AVX512VNNI
    if constexpr (bitsize == 512 && std::is_same_v<U1, i16> && std::is_same_v<U2, i16>) {
      return vector { _mm512_dpwssd_epi32(raw, first.raw, second.raw) };
    }
#endif
    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.accumulate_pair_dot(first.raw[0], second.raw[0]).raw;
      result.raw[1] = Half { raw[1] }.accumulate_pair_dot(first.raw[1], second.raw[1]).raw;
      return result;
    }
    return *this + first.pair_dot(second);
  }
}  // namespace lps2
