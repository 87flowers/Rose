#pragma once

#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"

#include <array>

namespace lps2::generic::vector_ops {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> test_vm(const vector<T, N>& first, const vector<T, N>& second) {
    vector_mask<T, N> result;
    std::array<T, N> first_raw = first.to_array();
    std::array<T, N> second_raw = second.to_array();
    for (usize i = 0; i < N; i++) {
      result.set(i, first_raw[i] & second_raw[i]);
    }
    return result;
  }
}  // namespace lps2::generic::vector_ops

namespace lps2 {
  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::test_vm(const vector<T, N>& second) const {
#if LPS2_ARCH_NEON
    if constexpr (bitsize == 128) {
      if constexpr (std::is_same_v<T, i8>) {
        return vmask_type { vtstq_s8(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i16>) {
        return vmask_type { vtstq_s16(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i32>) {
        return vmask_type { vtstq_s32(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, i64>) {
        return vmask_type { vtstq_s64(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u8>) {
        return vmask_type { vtstq_u8(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u16>) {
        return vmask_type { vtstq_u16(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u32>) {
        return vmask_type { vtstq_u32(raw, second.raw) };
      }
      if constexpr (std::is_same_v<T, u64>) {
        return vmask_type { vtstq_u64(raw, second.raw) };
      }
    }
#endif

    return (*this & second).nonzeros_vm();
  }

  template<class T, usize N>
  LPS2_INLINE constexpr bit_mask<T, N> vector<T, N>::test_bm(const vector<T, N>& second) const {
    return test_vm(second);
  }

  template<class T, usize N>
  LPS2_INLINE constexpr vector_mask<T, N> vector<T, N>::test(const vector<T, N>& second) const {
    return test_vm(second);
  }
}  // namespace lps2
