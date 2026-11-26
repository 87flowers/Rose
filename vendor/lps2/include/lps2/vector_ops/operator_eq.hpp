#pragma once

#include "lps2/arch.hpp"
#include "lps2/vector.hpp"

#include <bit>
#include <cstring>

namespace lps2 {
  template<class T, usize N>
  constexpr bool operator==(const vector<T, N>& first, const vector<T, N>& second) {
    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      return Half { first.raw[0] } == Half { second.raw[0] } && Half { first.raw[1] } == Half { second.raw[1] };
    }
#if LPS2_ARCH_X86_64
    else if constexpr (vector<T, N>::bitsize == 128) {
      __m128i x = _mm_cmpeq_epi8(first.raw, second.raw);
      return _mm_movemask_epi8(x) == 0xFFFF;
    }
#endif
#if LPS2_ARCH_SEE4_1
    else if constexpr (vector<T, N>::bitsize == 128) {
      __m128i x = _mm_xor_si128(first.raw, second.raw);
      return _mm_testz_si128(x, x);
    }
#endif
#if LPS2_ARCH_AVX2
    else if constexpr (vector<T, N>::bitsize == 256) {
      __m256i x = _mm256_xor_si256(first.raw, second.raw);
      return _mm256_testz_si256(x, x);
    }
#endif
#if LPS2_ARCH_AVX512BASE
    else if constexpr (vector<T, N>::bitsize == 512) {
      __mmask16 m = _mm512_cmpeq_epi32_mask(first.raw, second.raw);
      return _mm512_kortestc(m, m);
    }
#endif
    else {
      return std::memcmp(&first, &second, sizeof(first)) == 0;
    }
  }
}  // namespace lps2
