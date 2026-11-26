#pragma once

#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> zeros_vm(const vector<T, N>& first) {
    vector_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, !first_raw[i]);
    }
    return result;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr usize zeros_count(const vector<T, N>& first) {
    usize result = 0;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result += first_raw[i] == 0;
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::zeros_vm() const {
#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vmask_type { veqzq_s8(raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vmask_type { veqzq_s16(raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vmask_type { veqzq_s32(raw) };
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vmask_type { veqzq_s64(raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vmask_type { veqzq_u8(raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vmask_type { veqzq_u16(raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vmask_type { veqzq_u32(raw) };
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vmask_type { veqzq_u64(raw) };
      }
    }
#endif

    return eq(zero());
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::zeros_bm() const {
    return zeros_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::zeros() const {
    return zeros_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr usize vector<T, N>::zeros_count() const {
    return zeros_vm().popcount();
  }
}  // namespace lps2
