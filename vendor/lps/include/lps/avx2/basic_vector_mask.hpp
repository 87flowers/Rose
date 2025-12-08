#pragma once

#include "lps/avx2/basic_vector_mask.def.hpp"
#include "lps/avx2/vector.def.hpp"
#include "lps/detail/always_false.hpp"
#include "lps/generic/basic_vector_mask.def.hpp"
#include "lps/generic/vector.def.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <bit>
#include <cstring>
#include <immintrin.h>

namespace lps::avx2 {

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env>::basic_vector_mask(inner_type::raw_type value) :
      raw(value) {
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env>::basic_vector_mask(detail::bit_mask_base_t<N> value) {
    std::array<T, N> a;
    for (usize i = 0; i < N; i++) {
      a[i] = ((value >> i) & 1) ? true_value : false_value;
    }
    raw = std::bit_cast<inner_type>(a);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::zero() {
    basic_vector_mask<T, N, Env> result;
    result.raw = inner_type::zero();
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::splat(bool value) {
    basic_vector_mask<T, N, Env> result;
    result.raw = inner_type::splat(value ? true_value : false_value);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr void basic_vector_mask<T, N, Env>::set(usize index, bool value) {
    std::array<T, N> a = to_array();
    a[index] = value ? true_value : false_value;
    raw = std::bit_cast<inner_type>(a);
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_vector_mask<T, N, Env>::mask(const V& v1) const {
    return V { raw.raw } & v1;
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_vector_mask<T, N, Env>::select(const V& v0, const V& v1) const {
    if constexpr (V::is_128_bit) {
      return V { _mm_blendv_epi8(v0.raw, v1.raw, raw.raw) };
    } else {
      return V { _mm256_blendv_epi8(v0.raw, v1.raw, raw.raw) };
    }
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_vector_mask<T, N, Env>::compress(const V& v) const {
    using elem_t = typename V::element_type;

    auto src_array = v.to_array();
    auto mask_bits = this->to_bits();

    std::array<elem_t, N> result;
    size_t write_idx = 0;

    // Bit scan loop
    while (mask_bits) {
      size_t bit_pos = std::countr_zero(mask_bits);
      result[write_idx++] = src_array[bit_pos];
      mask_bits &= mask_bits - 1;
    }

    // Zero tail
    for (size_t i = write_idx; i < N; ++i) {
      result[i] = elem_t {};
    }

    return V { result };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::andnot(const basic_vector_mask<T, N, Env>& second) const {
    if constexpr (inner_type::is_128_bit) {
      return basic_vector_mask { _mm_andnot_si128(second.raw.raw, raw.raw) };
    } else {
      return basic_vector_mask { _mm256_andnot_si256(second.raw.raw, raw.raw) };
    }
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE usize basic_vector_mask<T, N, Env>::popcount() const {
    if constexpr (inner_type::is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<usize>(std::popcount(static_cast<u16>(_mm_movemask_epi8(raw.raw))));
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<usize>(std::popcount(static_cast<u16>(_mm_movemask_epi8(raw.raw) & 0xAAAA)));
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm_movemask_ps((__m128)raw.raw))));
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm_movemask_pd((__m128d)raw.raw))));
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<usize>(std::popcount(static_cast<u32>(_mm256_movemask_epi8(raw.raw))));
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<usize>(std::popcount(static_cast<u32>(_mm256_movemask_epi8(raw.raw)) & 0xAAAAAAAA));
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm256_movemask_ps((__m256)raw.raw))));
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<usize>(std::popcount(static_cast<u8>(_mm256_movemask_pd((__m256d)raw.raw))));
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE std::array<T, N> basic_vector_mask<T, N, Env>::to_array() const {
    return raw.to_array();
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE detail::bit_mask_base_t<N> basic_vector_mask<T, N, Env>::to_bits() const {
    if constexpr (inner_type::is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<u16>(_mm_movemask_epi8(raw.raw));
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<u8>(_pext_u32(static_cast<u32>(_mm_movemask_epi8(raw.raw)), 0xAAAA));
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<u8>(_mm_movemask_ps((__m128)raw.raw));
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<u8>(_mm_movemask_pd((__m128d)raw.raw));
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return static_cast<u32>(_mm256_movemask_epi8(raw.raw));
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return static_cast<u16>(_pext_u32(static_cast<u32>(_mm256_movemask_epi8(raw.raw)), 0xAAAAAAAA));
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return static_cast<u8>(_mm256_movemask_ps((__m256)raw.raw));
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return static_cast<u8>(_mm256_movemask_pd((__m256d)raw.raw));
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE basic_vector_mask<T, N, Env>::inner_type basic_vector_mask<T, N, Env>::to_vector() const {
    return raw;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr bool operator==(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first.raw == second.raw;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> operator~(const basic_vector_mask<T, N, Env>& first) {
    basic_vector_mask<T, N, Env> result;
    result.raw = ~first.raw;
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> operator&(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    basic_vector_mask<T, N, Env> result;
    result.raw = first.raw & second.raw;
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env>& operator&=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first = first & second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> operator|(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    basic_vector_mask<T, N, Env> result;
    result.raw = first.raw | second.raw;
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env>& operator|=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first = first | second;
  }

}  // namespace lps::avx2
