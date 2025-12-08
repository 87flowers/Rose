#pragma once

#include "lps/doubling/basic_vector_mask.def.hpp"
#include "lps/doubling/vector.def.hpp"
#include "lps/generic/basic_vector_mask.hpp"
#include "lps/generic/vector.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <bit>

namespace lps::doubling {

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env>::basic_vector_mask(detail::bit_mask_base_t<N> value) {
    raw[0] = base_type { static_cast<detail::bit_mask_base_t<N / 2>>(value) };
    raw[1] = base_type { static_cast<detail::bit_mask_base_t<N / 2>>(value >> (N / 2)) };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::zero() {
    basic_vector_mask<T, N, Env> result;
    result.raw[0] = base_type::zero();
    result.raw[1] = base_type::zero();
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::splat(bool value) {
    basic_vector_mask<T, N, Env> result;
    result.raw[0] = base_type::splat(value);
    result.raw[1] = base_type::splat(value);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr void basic_vector_mask<T, N, Env>::set(usize index, bool value) {
    raw[index / (N / 2)].set(index % (N / 2), value);
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_vector_mask<T, N, Env>::mask(const V& v1) const {
    V result;
    result.raw[0] = raw[0].mask(v1.raw[0]);
    result.raw[1] = raw[1].mask(v1.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_vector_mask<T, N, Env>::select(const V& v0, const V& v1) const {
    V result;
    result.raw[0] = raw[0].select(v0.raw[0], v1.raw[0]);
    result.raw[1] = raw[1].select(v0.raw[1], v1.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_vector_mask<T, N, Env>::compress(const V& v) const {
    return std::bit_cast<V>(
      std::bit_cast<generic::basic_vector_mask<T, N>>(*this).compress(std::bit_cast<generic::vector<typename V::element_type, N>>(v)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> basic_vector_mask<T, N, Env>::andnot(const basic_vector_mask<T, N, Env>& second) const {
    basic_vector_mask<T, N, Env> result;
    result.raw[0] = raw[0].andnot(second.raw[0]);
    result.raw[1] = raw[1].andnot(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr usize basic_vector_mask<T, N, Env>::popcount() const {
    return raw[0].popcount() + raw[1].popcount();
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr std::array<T, N> basic_vector_mask<T, N, Env>::to_array() const {
    return std::bit_cast<std::array<T, N>>(*this);
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr detail::bit_mask_base_t<N> basic_vector_mask<T, N, Env>::to_bits() const {
    detail::bit_mask_base_t<N> result = 0;
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[0].to_bits());
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[1].to_bits()) << (N / 2);
    return result;
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr basic_vector_mask<T, N, Env>::inner_type basic_vector_mask<T, N, Env>::to_vector() const {
    return std::bit_cast<basic_vector_mask<T, N, Env>::inner_type>(*this);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr bool operator==(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first.raw == second.raw;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> operator~(const basic_vector_mask<T, N, Env>& first) {
    basic_vector_mask<T, N, Env> result;
    result.raw[0] = ~first.raw[0];
    result.raw[1] = ~first.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> operator&(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    basic_vector_mask<T, N, Env> result;
    result.raw[0] = first.raw[0] & second.raw[0];
    result.raw[1] = first.raw[1] & second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env>& operator&=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first = first & second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env> operator|(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    basic_vector_mask<T, N, Env> result;
    result.raw[0] = first.raw[0] | second.raw[0];
    result.raw[1] = first.raw[1] | second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_vector_mask<T, N, Env>& operator|=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second) {
    return first = first | second;
  }

}  // namespace lps::doubling
