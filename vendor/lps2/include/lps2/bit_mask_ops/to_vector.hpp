#pragma once

#include "lps2/bit_mask.hpp"
#include "lps2/detail/vector_arch_type.hpp"
#include "lps2/vector_mask.hpp"

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector<T, N> basic_bit_mask<T, N>::to_vector() const {
    using vector_type = vector<T, N>;
#if LPS2_ARCH_AVX512BASE
    if constexpr (vector_type::bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector_type { _mm_movm_epi8(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector_type { _mm_movm_epi16(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector_type { _mm_movm_epi32(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector_type { _mm_movm_epi64(raw) };
      }
    }
    if constexpr (vector_type::bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector_type { _mm256_movm_epi8(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector_type { _mm256_movm_epi16(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector_type { _mm256_movm_epi32(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector_type { _mm256_movm_epi64(raw) };
      }
    }
    if constexpr (vector_type::bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector_type { _mm512_movm_epi8(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector_type { _mm512_movm_epi16(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vector_type { _mm512_movm_epi32(raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vector_type { _mm512_movm_epi64(raw) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      auto [m0, m1] = to_halves();
      vector<T, N> result;
      result.raw[0] = m0.to_vector().raw;
      result.raw[1] = m1.to_vector().raw;
      return result;
    }

    return vector_mask<T, N> { to_bits() }.to_vector();
  }
}  // namespace lps2
