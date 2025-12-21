#pragma once

#include "lps/generic/basic_vector_mask.def.hpp"
#include "lps/generic/vector.def.hpp"
#include "lps/stdint.hpp"

#include <array>

namespace lps::generic {
  template<class T, usize N>
  constexpr basic_vector_mask<T, N>::basic_vector_mask(detail::bit_mask_base_t<N> value) {
    for (usize i = 0; i < N; i++) {
      raw.raw[i] = ((value >> i) & 1) ? true_value : false_value;
    }
  }

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> basic_vector_mask<T, N>::zero() {
    basic_vector_mask result;
    result.raw = vector<T, N>::zero();
    return result;
  }

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> basic_vector_mask<T, N>::splat(bool value) {
    basic_vector_mask v;
    v.raw = inner_type::splat(value ? true_value : false_value);
    return v;
  }

  template<class T, usize N>
  constexpr void basic_vector_mask<T, N>::set(usize index, bool value) {
    raw.raw[index] = value ? true_value : false_value;
  }

  template<class T, usize N>
  template<class U>
  constexpr basic_vector_mask<detail::mask_element_t<U>, detail::clamped_size<U, N>> basic_vector_mask<T, N>::convert() const {
    return std::bit_cast<basic_vector_mask<detail::mask_element_t<U>, detail::clamped_size<U, N>>>(raw.template convert<detail::mask_element_t<U>>());
  }

  template<class T, usize N>
  template<class U>
    requires std::is_same_v<T, detail::mask_element_t<U>>
  constexpr vector<U, N> basic_vector_mask<T, N>::mask(const vector<U, N>& v1) const {
    return select(vector<U, N>::zero(), v1);
  }

  template<class T, usize N>
  template<class U>
    requires std::is_same_v<T, detail::mask_element_t<U>>
  constexpr vector<U, N> basic_vector_mask<T, N>::select(const vector<U, N>& v0, const vector<U, N>& v1) const {
    vector<U, N> result;
    for (usize i = 0; i < N; i++) {
      result.raw[i] = raw.raw[i] == false_value ? v0.raw[i] : v1.raw[i];
    }
    return result;
  }

  template<class T, usize N>
  template<class U>
    requires std::is_same_v<T, detail::mask_element_t<U>>
  constexpr vector<U, N> basic_vector_mask<T, N>::compress(const vector<U, N>& v) const {
    vector<U, N> result = vector<U, N>::zero();
    for (usize i = 0, j = 0; i < N; i++) {
      if (raw.raw[i] != false_value) {
        result.raw[j++] = v.raw[i];
      }
    }
    return result;
  }

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> basic_vector_mask<T, N>::andnot(const basic_vector_mask<T, N>& second) const {
    basic_vector_mask<T, N> result;
    result.raw = raw.andnot(second.raw);
    return result;
  }

  template<class T, usize N>
  [[nodiscard]] usize basic_vector_mask<T, N>::popcount() const {
    usize result = 0;
    for (usize i = 0; i < N; i++) {
      result += raw.raw[i] != false_value;
    }
    return result;
  }

  template<class T, usize N>
  [[nodiscard]] std::array<T, N> basic_vector_mask<T, N>::to_array() const {
    return raw;
  }

  template<class T, usize N>
  [[nodiscard]] detail::bit_mask_base_t<N> basic_vector_mask<T, N>::to_bits() const {
    detail::bit_mask_base_t<N> result = 0;
    for (usize i = 0; i < N; i++) {
      detail::bit_mask_base_t<N> bit = raw.raw[i] != false_value;
      result |= bit << i;
    }
    return result;
  }

  template<class T, usize N>
  [[nodiscard]] basic_vector_mask<T, N>::inner_type basic_vector_mask<T, N>::to_vector() const {
    return raw;
  }

  template<class T, usize N>
  constexpr bool operator==(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    return first.raw == second.raw;
  }

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> operator~(const basic_vector_mask<T, N>& first) {
    basic_vector_mask<T, N> result;
    result.raw = ~first.raw;
    return result;
  }

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> operator&(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    basic_vector_mask<T, N> result;
    result.raw = first.raw & second.raw;
    return result;
  }

  template<class T, usize N>
  constexpr basic_vector_mask<T, N>& operator&=(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    first.raw &= second.raw;
    return first;
  }

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> operator|(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    basic_vector_mask<T, N> result;
    result.raw = first.raw | second.raw;
    return result;
  }

  template<class T, usize N>
  constexpr basic_vector_mask<T, N>& operator|=(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    first.raw |= second.raw;
    return first;
  }

}  // namespace lps::generic
