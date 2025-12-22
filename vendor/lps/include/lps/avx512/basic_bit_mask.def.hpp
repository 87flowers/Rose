#pragma once

#include "lps/avx512/avx512.fwd.hpp"
#include "lps/detail/bit_mask_base.hpp"
#include "lps/detail/mask_element.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <bit>
#include <immintrin.h>
#include <type_traits>

namespace lps::detail {

  template<usize N>
  struct avx512_bit_mask_base;

  template<>
  struct avx512_bit_mask_base<2> {
    using type = __mmask8;
  };

  template<>
  struct avx512_bit_mask_base<4> {
    using type = __mmask8;
  };

  template<>
  struct avx512_bit_mask_base<8> {
    using type = __mmask8;
  };

  template<>
  struct avx512_bit_mask_base<16> {
    using type = __mmask16;
  };

  template<>
  struct avx512_bit_mask_base<32> {
    using type = __mmask32;
  };

  template<>
  struct avx512_bit_mask_base<64> {
    using type = __mmask64;
  };

  template<usize N>
  using avx512_bit_mask_base_t = typename avx512_bit_mask_base<N>::type;

}  // namespace lps::detail

namespace lps::avx512 {

  template<class T, usize N, class Env>
  struct basic_bit_mask {
    static constexpr usize size = N;
    static_assert(std::has_single_bit(N));

    using bits_type = detail::bit_mask_base_t<N>;
    using raw_type = detail::avx512_bit_mask_base_t<N>;

    constexpr basic_bit_mask() = default;
    explicit constexpr basic_bit_mask(raw_type value);
    explicit constexpr basic_bit_mask(bits_type value)
      requires(!std::is_same_v<raw_type, bits_type>);

    static constexpr basic_bit_mask zero();
    static constexpr basic_bit_mask splat(bool value);

    constexpr void set(usize index, bool value);

    template<class U>
    constexpr typename Env::template bit_mask<U, detail::clamped_size<U, N>> convert() const;

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
    [[nodiscard]] constexpr typename Env::template vector<T, N> to_vector() const;
    [[nodiscard]] constexpr typename Env::template vector_mask<T, N> to_vm() const;

    static constexpr T false_value = T { 0 };
    static constexpr T true_value = static_cast<T>(~T { 0 });

    raw_type raw;
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

  template<class T, usize N, class Env>
  constexpr basic_bit_mask<T, N, Env> operator^(const basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_bit_mask<T, N, Env>& operator^=(basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second);

}  // namespace lps::avx512
