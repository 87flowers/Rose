#pragma once

#include "lps/detail/bit_mask_base.hpp"
#include "lps/detail/mask_element.hpp"
#include "lps/doubling/doubling.fwd.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <type_traits>

namespace lps::doubling {

  template<class T, usize N, class Env>
  struct basic_bit_mask {
    static constexpr usize size = N;
    using vector_type = Env::template vector<T, N>;
    using vector_mask_type = Env::template vector_mask<T, N>;
    using base_type = Env::template bit_mask<T, N / 2>;
    using bits_type = detail::bit_mask_base_t<N>;

    constexpr basic_bit_mask() = default;
    explicit constexpr basic_bit_mask(detail::bit_mask_base_t<N> value);

    static constexpr basic_bit_mask zero();
    static constexpr basic_bit_mask splat(bool value);

    constexpr void set(usize index, bool value);

    template<class V>
      requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
    constexpr V mask(const V& v1) const;

    template<class V>
      requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
    constexpr V select(const V& v0, const V& v1) const;

    template<class V>
      requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
    constexpr V compress(const V& v) const;

    constexpr basic_bit_mask andnot(const basic_bit_mask& second) const;

    [[nodiscard]] constexpr usize popcount() const;

    [[nodiscard]] constexpr std::array<T, N> to_array() const;
    [[nodiscard]] constexpr bits_type to_bits() const;
    [[nodiscard]] constexpr vector_mask_type to_vm() const;
    [[nodiscard]] constexpr vector_type to_vector() const;

    static constexpr T false_value = T { 0 };
    static constexpr T true_value = static_cast<T>(~T { 0 });

    std::array<base_type, 2> raw;
  };

  template<class T, usize N, class Env>
  constexpr bool operator==(const basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_bit_mask<T, N, Env> operator~(const basic_bit_mask<T, N, Env>& first);

  template<class T, usize N, class Env>
  constexpr basic_bit_mask<T, N, Env> operator&(const basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_bit_mask<T, N, Env>& operator&=(basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_bit_mask<T, N, Env> operator|(const basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_bit_mask<T, N, Env>& operator|=(basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second);

}  // namespace lps::doubling
