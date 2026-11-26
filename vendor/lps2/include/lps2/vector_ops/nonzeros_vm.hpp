#pragma once

#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> nonzeros_vm(const vector<T, N>& first) {
    vector_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, first_raw[i]);
    }
    return result;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr usize nonzeros_count(const vector<T, N>& first) {
    usize result = 0;
    std::array<T, N> first_raw = first.to_array();
    for (usize i = 0; i < N; i++) {
      result += first_raw[i] != 0;
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::nonzeros_vm() const {
#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vmask_type { vtstq_s8(raw, raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vmask_type { vtstq_s16(raw, raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vmask_type { vtstq_s32(raw, raw) };
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vmask_type { vtstq_s64(raw, raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vmask_type { vtstq_u8(raw, raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vmask_type { vtstq_u16(raw, raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vmask_type { vtstq_u32(raw, raw) };
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vmask_type { vtstq_u64(raw, raw) };
      }
    }
#endif

    return neq(zero());
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::nonzeros_bm() const {
    return nonzeros_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::nonzeros() const {
    return nonzeros_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr usize vector<T, N>::nonzeros_count() const {
    return nonzeros_vm().popcount();
  }
}  // namespace lps2
