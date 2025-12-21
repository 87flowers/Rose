#pragma once

#include "lps/detail/bit_mask_base.hpp"
#include "lps/detail/mask_element.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/doubling/doubling.fwd.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <type_traits>

namespace lps::doubling {

  template<class T, usize N, class Env>
  struct basic_vector_mask {
    static constexpr bool is_128_bit = N * sizeof(T) == 16 * sizeof(u8);
    static constexpr bool is_256_bit = N * sizeof(T) == 32 * sizeof(u8);
    static constexpr bool is_512_bit = N * sizeof(T) == 64 * sizeof(u8);

    static constexpr usize size = N;
    using inner_type = Env::template vector<T, N>;
    using base_type = Env::template vector_mask<T, N / 2>;

    constexpr basic_vector_mask() = default;
    explicit constexpr basic_vector_mask(detail::bit_mask_base_t<N> value);

    static constexpr basic_vector_mask zero();
    static constexpr basic_vector_mask splat(bool value);

    constexpr void set(usize index, bool value);

    template<class U>
    constexpr typename Env::template vector_mask<U, detail::clamped_size<U, N>> convert() const;

    template<class V>
      requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
    constexpr V mask(const V& v1) const;

    template<class V>
      requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
    constexpr V select(const V& v0, const V& v1) const;

    template<class V>
      requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
    constexpr V compress(const V& v) const;

    constexpr basic_vector_mask andnot(const basic_vector_mask& second) const;

    [[nodiscard]] constexpr usize popcount() const;

    [[nodiscard]] constexpr std::array<T, N> to_array() const;
    [[nodiscard]] constexpr detail::bit_mask_base_t<N> to_bits() const;
    [[nodiscard]] constexpr inner_type to_vector() const;

    static constexpr T false_value = T { 0 };
    static constexpr T true_value = static_cast<T>(~T { 0 });

    std::array<base_type, 2> raw;
  };

  template<class T, usize N, class Env>
  constexpr bool operator==(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> operator~(const basic_vector_mask<T, N, Env>& first);

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> operator&(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env>& operator&=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> operator|(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env>& operator|=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second);

}  // namespace lps::doubling
