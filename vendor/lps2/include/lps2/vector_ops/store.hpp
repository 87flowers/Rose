#pragma once

#include "lps2/stdint.hpp"
#include "lps2/vector.hpp"

#include <cstring>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr void store(const vector<T, N>& self, void* dst) {
    std::memcpy(dst, &self, sizeof(self));
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr void vector<T, N>::store(void* dst) const {
#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), raw);
      return;
    }
#endif

#if LPS2_ARCH_AVX
    if constexpr (bitsize == 256) {
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst), raw);
      return;
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      _mm512_storeu_si512(reinterpret_cast<__m512i*>(dst), raw);
      return;
    }
#endif

    generic::vector_ops::store(*this, dst);
  }
}  // namespace lps2
