#pragma once

#include "lps/avx512/basic_vector_mask.def.hpp"
#include "lps/avx512/vector.def.hpp"
#include "lps/detail/always_false.hpp"
#include "lps/generic/basic_vector_mask.def.hpp"
#include "lps/generic/vector.def.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <bit>

namespace lps::avx512 {

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
    } else if constexpr (V::is_256_bit) {
      return V { _mm256_blendv_epi8(v0.raw, v1.raw, raw.raw) };
    } else {
      return V { _mm512_mask_blend_epi8(_mm512_movepi8_mask(v0.raw), v1.raw, raw.raw) };
    }
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_vector_mask<T, N, Env>::compress(const V& v) const {
    if constexpr (V::is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm_maskz_compress_epi8(_mm_movepi8_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm_maskz_compress_epi16(_mm_movepi16_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm_maskz_compress_epi32(_mm_movepi32_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm_maskz_compress_epi64(_mm_movepi64_mask(raw.raw), v.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (V::is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm256_maskz_compress_epi8(_mm256_movepi8_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm256_maskz_compress_epi16(_mm256_movepi16_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm256_maskz_compress_epi32(_mm256_movepi32_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm256_maskz_compress_epi64(_mm256_movepi64_mask(raw.raw), v.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm512_maskz_compress_epi8(_mm512_movepi8_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm512_maskz_compress_epi16(_mm512_movepi16_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm512_maskz_compress_epi32(_mm512_movepi32_mask(raw.raw), v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm512_maskz_compress_epi64(_mm512_movepi64_mask(raw.raw), v.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::andnot(const basic_vector_mask<T, N, Env>& second) const {
    if constexpr (inner_type::is_128_bit) {
      return basic_vector_mask { _mm_andnot_si128(second.raw.raw, raw.raw) };
    } else if constexpr (inner_type::is_256_bit) {
      return basic_vector_mask { _mm256_andnot_si256(second.raw.raw, raw.raw) };
    } else {
      return basic_vector_mask { _mm512_andnot_si512(second.raw.raw, raw.raw) };
    }
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr usize basic_vector_mask<T, N, Env>::popcount() const {
    return std::popcount(to_bits());
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr std::array<T, N> basic_vector_mask<T, N, Env>::to_array() const {
    return raw.to_array();
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr detail::bit_mask_base_t<N> basic_vector_mask<T, N, Env>::to_bits() const {
    return std::bit_cast<generic::basic_vector_mask<T, N>>(*this).to_bits();
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr basic_vector_mask<T, N, Env>::inner_type basic_vector_mask<T, N, Env>::to_vector() const {
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

}  // namespace lps::avx512
