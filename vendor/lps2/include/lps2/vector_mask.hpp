#pragma once

#include "lps2/arch.hpp"
#include "lps2/detail/bit_mask_base.hpp"
#include "lps2/detail/mask_element.hpp"
#include "lps2/detail/vector_clamped_size.hpp"
#include "lps2/lps2.fwd.hpp"
#include "lps2/stdint.hpp"

#include <array>
#include <bit>

namespace lps2 {

  template<class T, usize N>
  struct basic_vector_mask {
    static constexpr usize size = N;
    using inner_type = vector<T, N>;

    constexpr basic_vector_mask() = default;
    explicit constexpr basic_vector_mask(inner_type::raw_type value) :
        raw(value) {};
    explicit constexpr basic_vector_mask(typename vector<std::make_unsigned_t<T>, N>::raw_type value)
      requires(!std::is_same_v<typename vector<std::make_unsigned_t<T>, N>::raw_type, typename inner_type::raw_type>)
        :
        raw(std::bit_cast<typename inner_type::raw_type>(value)) {};
    explicit constexpr basic_vector_mask(detail::bit_mask_base_t<N> value);

    static constexpr basic_vector_mask zero();
    static constexpr basic_vector_mask splat(bool value);

    constexpr void set(usize index, bool value);

    template<class U>
    constexpr basic_vector_mask<detail::mask_element_t<U>, detail::clamped_size<U, N>> convert() const;

    template<class V>
      requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
    constexpr V mask(const V& v1) const;

    template<class V>
      requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
    constexpr V select(const V& v0, const V& v1) const;

    template<class V>
      requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
    constexpr V compress(const V& v) const;

    constexpr basic_vector_mask andnot(const basic_vector_mask& second) const;

    [[nodiscard]] constexpr usize popcount() const;

    [[nodiscard]] LPS2_INLINE constexpr std::array<T, N> to_array() const {
      return raw.to_array();
    }

    [[nodiscard]] constexpr detail::bit_mask_base_t<N> to_bits() const;

    [[nodiscard]] LPS2_INLINE constexpr inner_type to_vector() const {
      return raw;
    }

    static constexpr T false_value = T { 0 };
    static constexpr T true_value = static_cast<T>(~T { 0 });

    inner_type raw;
  };

  template<class T, usize N>
  LPS2_INLINE constexpr bool operator==(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    return first.raw == second.raw;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N> operator~(const basic_vector_mask<T, N>& first) {
    basic_vector_mask<T, N> result;
    result.raw = ~first.raw;
    return result;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N> operator&(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    basic_vector_mask<T, N> result;
    result.raw = first.raw & second.raw;
    return result;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N>& operator&=(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    first.raw &= second.raw;
    return first;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N> operator|(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    basic_vector_mask<T, N> result;
    result.raw = first.raw | second.raw;
    return result;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N>& operator|=(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    first.raw |= second.raw;
    return first;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N> operator^(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    basic_vector_mask<T, N> result;
    result.raw = first.raw ^ second.raw;
    return result;
  }

  template<class T, usize N>
  LPS2_INLINE constexpr basic_vector_mask<T, N>& operator^=(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second) {
    first.raw ^= second.raw;
    return first;
  }

}  // namespace lps2
