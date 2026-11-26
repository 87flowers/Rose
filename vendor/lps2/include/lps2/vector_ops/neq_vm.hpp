#pragma once

#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> neq_vm(const vector<T, N>& first, const vector<T, N>& second) {
    vector_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    std::array<T, N> second_raw = second.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, first_raw[i] != second_raw[i]);
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::neq_vm(const vector<T, N>& second) const {
#if LPS2_ARCH_X86_64
    if constexpr (bitsize == 128) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return ~vmask_type { _mm_cmpeq_epi8(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return ~vmask_type { _mm_cmpeq_epi16(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return ~vmask_type { _mm_cmpeq_epi32(raw, second.raw) };
      }
#if LPS2_ARCH_SSE4_1
      if constexpr (sizeof(T) == sizeof(u64)) {
        return ~vmask_type { _mm_cmpeq_epi64(raw, second.raw) };
      }
#endif
    }
#endif

#if LPS2_ARCH_AVX2
    if constexpr (bitsize == 256) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return ~vmask_type { _mm256_cmpeq_epi8(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return ~vmask_type { _mm256_cmpeq_epi16(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return ~vmask_type { _mm256_cmpeq_epi32(raw, second.raw) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return ~vmask_type { _mm256_cmpeq_epi64(raw, second.raw) };
      }
    }
#endif

#if LPS2_ARCH_AVX512BASE
    if constexpr (bitsize == 512) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vmask_type { _mm512_movm_epi8(_mm512_cmpneq_epu8_mask(raw, second.raw)) };
      }
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vmask_type { _mm512_movm_epi16(_mm512_cmpneq_epu16_mask(raw, second.raw)) };
      }
      if constexpr (sizeof(T) == sizeof(u32)) {
        return vmask_type { _mm512_movm_epi32(_mm512_cmpneq_epu32_mask(raw, second.raw)) };
      }
      if constexpr (sizeof(T) == sizeof(u64)) {
        return vmask_type { _mm512_movm_epi64(_mm512_cmpneq_epu64_mask(raw, second.raw)) };
      }
    }
#endif

#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vmask_type { vceqzq_s8(vceqq_s8(raw, second.raw)) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vmask_type { vceqzq_s16(vceqq_s16(raw, second.raw)) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vmask_type { vceqzq_s32(vceqq_s32(raw, second.raw)) };
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vmask_type { vceqzq_s64(vceqq_s64(raw, second.raw)) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vmask_type { vceqzq_u8(vceqq_u8(raw, second.raw)) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vmask_type { vceqzq_u16(vceqq_u16(raw, second.raw)) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vmask_type { vceqzq_u32(vceqq_u32(raw, second.raw)) };
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vmask_type { vceqzq_u64(vceqq_u64(raw, second.raw)) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector_mask<T, N> result;
      result.raw.raw[0] = Half { raw[0] }.neq(second.raw[0]).raw.raw;
      result.raw.raw[1] = Half { raw[1] }.neq(second.raw[1]).raw.raw;
      return result;
    }

    return generic::vector_ops::neq_vm(*this, second);
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::neq_bm(const vector<T, N>& second) const {
    return neq_vm(second);
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::neq(const vector<T, N>& second) const {
    return neq_vm(second);
  }
}  // namespace lps2
