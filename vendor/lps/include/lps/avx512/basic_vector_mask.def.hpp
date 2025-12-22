#pragma once

#include "lps/avx512/avx512.fwd.hpp"
#include "lps/detail/bit_mask_base.hpp"
#include "lps/detail/mask_element.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <type_traits>

namespace lps::avx512 {

  template<class T, usize N, class Env>
  struct basic_vector_mask {
    static constexpr usize size = N;
    using inner_type = Env::template vector<T, N>;

    constexpr basic_vector_mask() = default;
    explicit constexpr basic_vector_mask(inner_type::raw_type value);
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

    inner_type raw;
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

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env> operator^(const basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr basic_vector_mask<T, N, Env>& operator^=(basic_vector_mask<T, N, Env>& first, const basic_vector_mask<T, N, Env>& second);

}  // namespace lps::avx512
