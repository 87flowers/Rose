#pragma once

#include "lps2/vector.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N, usize M>
    requires(M != N)
  LPS2_INLINE constexpr vector<T, N> swizzle(const vector<T, N>& self, const vector<T, M>& src) {
    std::array<T, N> result_raw;
    std::array<T, N> self_raw = self.to_array();
    std::array<T, M> src_raw = src.to_array();
    for (usize i = 0; i < N; i++) {
      result_raw[i] = self_raw[i] < M ? src_raw[self_raw[i]] : 0;
    }
    return std::bit_cast<vector<T, N>>(result_raw);
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> vector<T, N>::swizzle(const vector<T, 16 / sizeof(T)>& src) const
    requires(16 / sizeof(T) != N)
  {
#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(src.raw), raw) };
      } else {
        return swizzle(vector<T, N> { _mm256_broadcastsi128_si256(src.raw) });
      }
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm512_shuffle_epi8(_mm512_broadcast_i32x4(src.raw), raw) };
      } else {
        return swizzle(vector<T, N> { _mm512_broadcast_i32x4(src.raw) });
      }
    }
#endif

#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vector { vqtbl1q_s8(src.raw, raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vector { vqtbl1q_u8(src.raw, raw) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector<T, N> result;
      result.raw[0] = Half { raw[0] }.swizzle(src).raw;
      result.raw[1] = Half { raw[1] }.swizzle(src).raw;
      return result;
    }

    return generic::vector_ops::swizzle(*this, src);
  }
}  // namespace lps2
