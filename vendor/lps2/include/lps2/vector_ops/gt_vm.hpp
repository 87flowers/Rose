#pragma once

#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> gt_vm(const vector<T, N>& first, const vector<T, N>& second) {
    vector_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    std::array<T, N> second_raw = second.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, first_raw[i] > second_raw[i]);
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::gt_vm(const vector<T, N>& second) const {
    if constexpr (std::is_signed_v<T>) {
#if LPS2_ARCH_X86_64
      if constexpr (bitsize == 128) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return vmask_type { _mm_cmpgt_epi8(raw, second.raw) };
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vmask_type { _mm_cmpgt_epi16(raw, second.raw) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vmask_type { _mm_cmpgt_epi32(raw, second.raw) };
        }
#if LPS2_ARCH_SSE4_2
        if constexpr (sizeof(T) == sizeof(u64)) {
          return vmask_type { _mm_cmpgt_epi64(raw, second.raw) };
        }
#endif
      }
#endif

#if LPS2_ARCH_AVX2
      if constexpr (bitsize == 256) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return vmask_type { _mm256_cmpgt_epi8(raw, second.raw) };
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vmask_type { _mm256_cmpgt_epi16(raw, second.raw) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vmask_type { _mm256_cmpgt_epi32(raw, second.raw) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
          return vmask_type { _mm256_cmpgt_epi64(raw, second.raw) };
        }
      }
#endif

#if LPS2_ARCH_AVX512BASE
      if constexpr (bitsize == 512) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return vmask_type { _mm512_movm_epi8(_mm512_cmpgt_epi8_mask(raw, second.raw)) };
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vmask_type { _mm512_movm_epi16(_mm512_cmpgt_epi16_mask(raw, second.raw)) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vmask_type { _mm512_movm_epi32(_mm512_cmpgt_epi32_mask(raw, second.raw)) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
          return vmask_type { _mm512_movm_epi64(_mm512_cmpgt_epi64_mask(raw, second.raw)) };
        }
      }
#endif
    }

    if constexpr (!std::is_signed_v<T>) {
#if LPS2_ARCH_X86_64
      if constexpr (bitsize == 128) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return ~vmask_type { _mm_cmpeq_epi8(_mm_min_epu8(raw, second.raw), raw) };
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return ~vmask_type { _mm_cmpeq_epi16(_mm_min_epu16(raw, second.raw), raw) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return ~vmask_type { _mm_cmpeq_epi32(_mm_min_epu32(raw, second.raw), raw) };
        }
#if LPS2_ARCH_AVX512BASE
        if constexpr (sizeof(T) == sizeof(u64)) {
          return ~vmask_type { _mm_cmpeq_epi64(_mm_min_epu64(raw, second.raw), raw) };
        }
#endif
      }
#endif

#if LPS2_ARCH_AVX2
      if constexpr (bitsize == 256) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return ~vmask_type { _mm256_cmpeq_epi8(_mm256_min_epu8(raw, second.raw), raw) };
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return ~vmask_type { _mm256_cmpeq_epi16(_mm256_min_epu16(raw, second.raw), raw) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return ~vmask_type { _mm256_cmpeq_epi32(_mm256_min_epu32(raw, second.raw), raw) };
        }
#if LPS2_ARCH_AVX512BASE
        if constexpr (sizeof(T) == sizeof(u64)) {
          return ~vmask_type { _mm256_cmpeq_epi64(_mm256_min_epu64(raw, second.raw), raw) };
        }
#endif
      }
#endif

#if LPS2_ARCH_AVX512BASE
      if constexpr (bitsize == 512) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return vmask_type { _mm512_movm_epi8(_mm512_cmpgt_epu8_mask(raw, second.raw)) };
        }
        if constexpr (sizeof(T) == sizeof(u16)) {
          return vmask_type { _mm512_movm_epi16(_mm512_cmpgt_epu16_mask(raw, second.raw)) };
        }
        if constexpr (sizeof(T) == sizeof(u32)) {
          return vmask_type { _mm512_movm_epi32(_mm512_cmpgt_epu32_mask(raw, second.raw)) };
        }
        if constexpr (sizeof(T) == sizeof(u64)) {
          return vmask_type { _mm512_movm_epi64(_mm512_cmpgt_epu64_mask(raw, second.raw)) };
        }
      }
#endif
    }

#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vmask_type { vcgtq_s8(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vmask_type { vcgtq_s16(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vmask_type { vcgtq_s32(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vmask_type { vcgtq_s64(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vmask_type { vcgtq_u8(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vmask_type { vcgtq_u16(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vmask_type { vcgtq_u32(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vmask_type { vcgtq_u64(raw, second.raw) };
      }
    }
#endif

    if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
      using Half = vector<T, N / 2>;
      vector_mask<T, N> result;
      result.raw.raw[0] = Half { raw[0] }.gt(second.raw[0]).raw.raw;
      result.raw.raw[1] = Half { raw[1] }.gt(second.raw[1]).raw.raw;
      return result;
    }

    return generic::vector_ops::gt_vm(*this, second);
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::gt_bm(const vector<T, N>& second) const {
    return gt_vm(second);
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::gt(const vector<T, N>& second) const {
    return gt_vm(second);
  }
}  // namespace lps2
