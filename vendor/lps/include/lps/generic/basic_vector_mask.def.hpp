#pragma once

#include "lps/detail/bit_mask_base.hpp"
#include "lps/detail/mask_element.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/generic/generic.fwd.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <type_traits>

namespace lps::generic {

  template<class T, usize N>
  constexpr bool operator==(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> operator~(const basic_vector_mask<T, N>& first);

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> operator&(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_vector_mask<T, N>& operator&=(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> operator|(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_vector_mask<T, N>& operator|=(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_vector_mask<T, N> operator^(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_vector_mask<T, N>& operator^=(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

  template<class T, usize N>
  struct basic_vector_mask {
    static constexpr usize size = N;
    using inner_type = vector<T, N>;

    constexpr basic_vector_mask() = default;
    explicit constexpr basic_vector_mask(detail::bit_mask_base_t<N> value);

    static constexpr basic_vector_mask zero();
    static constexpr basic_vector_mask splat(bool value);

    constexpr void set(usize index, bool value);

    template<class U>
    constexpr basic_vector_mask<detail::mask_element_t<U>, detail::clamped_size<U, N>> convert() const;

    template<class U>
      requires std::is_same_v<T, detail::mask_element_t<U>>
    constexpr vector<U, N> mask(const vector<U, N>& v1) const;

    template<class U>
      requires std::is_same_v<T, detail::mask_element_t<U>>
    constexpr vector<U, N> select(const vector<U, N>& v0, const vector<U, N>& v1) const;

    template<class U>
      requires std::is_same_v<T, detail::mask_element_t<U>>
    constexpr vector<U, N> compress(const vector<U, N>& v) const;

    constexpr basic_vector_mask andnot(const basic_vector_mask& second) const;

    [[nodiscard]] usize popcount() const;

    [[nodiscard]] std::array<T, N> to_array() const;
    [[nodiscard]] detail::bit_mask_base_t<N> to_bits() const;
    [[nodiscard]] inner_type to_vector() const;

    friend constexpr bool operator== <T, N>(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

    friend constexpr basic_vector_mask<T, N> operator~ <T, N>(const basic_vector_mask<T, N>& first);

    friend constexpr basic_vector_mask<T, N> operator& <T, N>(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

    friend constexpr basic_vector_mask<T, N>& operator&= <T, N>(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

    friend constexpr basic_vector_mask<T, N> operator| <T, N>(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

    friend constexpr basic_vector_mask<T, N>& operator|= <T, N>(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

    friend constexpr basic_vector_mask<T, N> operator^ <T, N>(const basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

    friend constexpr basic_vector_mask<T, N>& operator^= <T, N>(basic_vector_mask<T, N>& first, const basic_vector_mask<T, N>& second);

    template<class U, usize M>
    friend struct vector;
  private:
    static constexpr T false_value = T { 0 };
    static constexpr T true_value = T { -1 };

    inner_type raw;
  };

}  // namespace lps::generic
