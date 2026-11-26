#pragma once

#include "lps2/arch.hpp"
#include "lps2/detail/bit_mask_arch_type.hpp"
#include "lps2/detail/bit_mask_base.hpp"
#include "lps2/detail/mask_element.hpp"
#include "lps2/detail/vector_clamped_size.hpp"
#include "lps2/lps2.fwd.hpp"
#include "lps2/stdint.hpp"

#include <array>
#include <bit>

namespace lps2 {

  template<class T, usize N>
  struct basic_bit_mask {
    static constexpr usize size = N;
    static_assert(std::has_single_bit(N));

    using bits_type = detail::bit_mask_base_t<N>;
    using raw_type = detail::bit_mask_arch_type_t<T, N>;

    constexpr basic_bit_mask() = default;

    LPS2_INLINE explicit constexpr basic_bit_mask(raw_type value) :
        raw(value) {
    }

    explicit constexpr basic_bit_mask(bits_type value)
      requires(!std::is_same_v<raw_type, bits_type>);

    [[nodiscard]] LPS2_INLINE constexpr std::array<basic_bit_mask<T, N / 2>, 2> to_halves() const
      requires(N >= 2)
    {
      return std::bit_cast<std::array<basic_bit_mask<T, N / 2>, 2>>(*this);
    }

    static constexpr basic_bit_mask zero();
    static constexpr basic_bit_mask splat(bool value);
    static constexpr basic_bit_mask concat(basic_bit_mask<T, N / 2> lower, basic_bit_mask<T, N / 2> upper);

    constexpr void set(usize index, bool value);

    template<class U>
    constexpr bit_mask<U, detail::clamped_size<U, N>> convert() const;

    template<class V>
      requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
    constexpr V mask(const V& v1) const;

    template<class V>
      requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
    constexpr V select(const V& v0, const V& v1) const;

    template<class V>
      requires std::is_same_v<V, vector<typename V::element_type, N>> && std::is_same_v<T, detail::mask_element_t<typename V::element_type>>
    constexpr V compress(const V& v) const;

    constexpr basic_bit_mask andnot(const basic_bit_mask& second) const;

    [[nodiscard]] constexpr usize popcount() const;

    [[nodiscard]] constexpr bits_type to_bits() const;

    [[nodiscard]] constexpr vector<T, N> to_vector() const;

    [[nodiscard]] constexpr vector_mask<T, N> to_vm() const {
      vector_mask<T, N> result;
      result.raw = to_vector();
      return result;
    }

    [[nodiscard]] constexpr std::array<T, N> to_array() const {
      return to_vector().to_array();
    }

    static constexpr T false_value = T { 0 };
    static constexpr T true_value = static_cast<T>(~T { 0 });

    raw_type raw;
  };

  template<class T, usize N>
  constexpr bool operator==(const basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_bit_mask<T, N> operator~(const basic_bit_mask<T, N>& first);

  template<class T, usize N>
  constexpr basic_bit_mask<T, N> operator&(const basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_bit_mask<T, N>& operator&=(basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_bit_mask<T, N> operator|(const basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_bit_mask<T, N>& operator|=(basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_bit_mask<T, N> operator^(const basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second);

  template<class T, usize N>
  constexpr basic_bit_mask<T, N>& operator^=(basic_bit_mask<T, N>& first, const basic_bit_mask<T, N>& second);

}  // namespace lps2
