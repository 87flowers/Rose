#pragma once

#include "lps/detail/always_false.hpp"
#include "lps/generic/basic_vector_mask.def.hpp"
#include "lps/generic/vector.def.hpp"
#include "lps/sse4_2/basic_vector_mask.def.hpp"
#include "lps/sse4_2/vector.def.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <nmmintrin.h>

namespace lps::sse4_2 {

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env>::basic_vector_mask(__m128i value) :
      raw(value) {
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env>::basic_vector_mask(detail::bit_mask_base_t<N> value) {
    std::array<T, N> a;
    for (usize i = 0; i < N; i++) {
      a[i] = ((value >> i) & 1) ? true_value : false_value;
    }
    raw = std::bit_cast<inner_type>(a);
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::zero() {
    basic_vector_mask<T, N, Env> result;
    result.raw[0] = inner_type::zero();
    return result;
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::splat(bool value) {
    basic_vector_mask<T, N, Env> result;
    result.raw = inner_type::splat(value ? true_value : false_value);
    return result;
  }

  template<class T, usize N, class Env>
  constexpr void basic_vector_mask<T, N, Env>::set(usize index, bool value) {
    std::array<T, N> a = to_array();
    a[index] = value ? true_value : false_value;
    raw = std::bit_cast<inner_type>(a);
  }

  template<class T, usize N, class Env>
  template<class U>
  constexpr typename Env::template vector_mask<U, detail::clamped_size<U, N>> basic_vector_mask<T, N, Env>::convert() const {
    return std::bit_cast<typename Env::template vector_mask<U, detail::clamped_size<U, N>>>(raw.template convert<U>());
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  constexpr V basic_vector_mask<T, N, Env>::mask(const V& v1) const {
    return select(V::zero(), v1);
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  constexpr V basic_vector_mask<T, N, Env>::select(const V& v0, const V& v1) const {
    return V { _mm_blendv_epi8(v0.raw, v1.raw, raw.raw) };
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  constexpr V basic_vector_mask<T, N, Env>::compress(const V& v) const {
    return std::bit_cast<V>(
      std::bit_cast<generic::basic_vector_mask<T, N>>(*this).compress(std::bit_cast<generic::vector<typename V::element_type, N>>(v)));
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::andnot(const basic_vector_mask<T, N, Env>& second) const {
    return basic_vector_mask { _mm_andnot_si128(second.raw.raw, raw.raw) };
  }

  template<class T, usize N, class Env>
  [[nodiscard]] usize basic_vector_mask<T, N, Env>::popcount() const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      return static_cast<usize>(std::popcount(static_cast<u16>(_mm_movemask_epi8(raw.raw))));
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return static_cast<usize>(std::popcount(static_cast<u16>(_mm_movemask_epi8(_mm_packus_epi16(raw.template shr<8>().raw, _mm_setzero_si128())))));
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return static_cast<usize>(std::popcount(static_cast<u8>(_mm_movemask_ps((__m128)raw.raw))));
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return static_cast<usize>(std::popcount(static_cast<u8>(_mm_movemask_pd((__m128d)raw.raw))));
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  [[nodiscard]] std::array<T, N> basic_vector_mask<T, N, Env>::to_array() const {
    return std::bit_cast<std::array<T, N>>(*this);
  }

  template<class T, usize N, class Env>
  [[nodiscard]] detail::bit_mask_base_t<N> basic_vector_mask<T, N, Env>::to_bits() const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      return static_cast<u16>(_mm_movemask_epi8(raw.raw));
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return static_cast<u8>(_mm_movemask_epi8(_mm_packus_epi16(raw.template shr<8>().raw, _mm_setzero_si128())));
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return static_cast<u8>(_mm_movemask_ps((__m128)raw.raw));
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return static_cast<u8>(_mm_movemask_pd((__m128d)raw.raw));
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  [[nodiscard]] basic_vector_mask<T, N, Env>::inner_type basic_vector_mask<T, N, Env>::to_vector() const {
    return std::bit_cast<basic_vector_mask<T, N, Env>::inner_type>(*this);
  }

  template<class T, usize N, class Env>
  constexpr bool operator==(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first.raw == second.raw;
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> operator~(const basic_vector_mask<T, N, Env>& first) {
    basic_vector_mask<T, N, Env> result;
    result.raw = ~first.raw;
    return result;
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> operator&(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    basic_vector_mask<T, N, Env> result;
    result.raw = first.raw & second.raw;
    return result;
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env>& operator&=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first = first & second;
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> operator|(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    basic_vector_mask<T, N, Env> result;
    result.raw = first.raw | second.raw;
    return result;
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env>& operator|=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first = first | second;
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> operator^(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    basic_vector_mask<T, N, Env> result;
    result.raw = first.raw ^ second.raw;
    return result;
  }

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env>& operator^=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first = first ^ second;
  }

}  // namespace lps::sse4_2
