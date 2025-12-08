#pragma once

#include "lps/avx512/avx512.fwd.hpp"
#include "lps/avx512/basic_bit_mask.hpp"
#include "lps/avx512/basic_vector_mask.hpp"
#include "lps/avx512/vector.hpp"
#include "lps/doubling/basic_bit_mask.hpp"
#include "lps/doubling/basic_vector_mask.hpp"
#include "lps/doubling/doubling.fwd.hpp"
#include "lps/doubling/vector.hpp"
#include "lps/stdint.hpp"

#include <type_traits>

namespace lps::avx512 {

  struct environment;

  namespace detail {
    template<class T, usize N>
    struct vector_helper {
      using type = std::conditional_t<
        (sizeof(T) * N < 16 * sizeof(u8)), void,
        std::conditional_t<(sizeof(T) * N <= 64 * sizeof(u8)), avx512::vector<T, N, environment>, doubling::vector<T, N, environment>>>;
    };

    template<class T>
    struct vector_helper<T, 0> {
      using type = void;
    };

    template<class T, usize N>
    struct vector_mask_helper {
      using type = std::conditional_t<
        (sizeof(T) * N < 16 * sizeof(u8)), void,
        std::conditional_t<(sizeof(T) * N <= 64 * sizeof(u8)), avx512::vector_mask<T, N, environment>, doubling::vector_mask<T, N, environment>>>;
    };

    template<class T>
    struct vector_mask_helper<T, 0> {
      using type = void;
    };

    template<class T, usize N>
    struct bit_mask_helper {
      using type = std::conditional_t<
        (sizeof(T) * N < 16 * sizeof(u8)), void,
        std::conditional_t<(sizeof(T) * N <= 64 * sizeof(u8)), avx512::bit_mask<T, N, environment>, doubling::bit_mask<T, N, environment>>>;
    };

    template<class T>
    struct bit_mask_helper<T, 0> {
      using type = void;
    };
  }  // namespace detail

  struct environment {
    static constexpr bool prefer_bm = true;

    template<class T, usize N>
    using vector = typename detail::vector_helper<T, N>::type;

    template<class T, usize N>
    using vector_mask = typename detail::vector_mask_helper<T, N>::type;

    template<class T, usize N>
    using bit_mask = typename detail::bit_mask_helper<T, N>::type;

    template<class T, usize N>
    using mask = typename detail::bit_mask_helper<T, N>::type;
  };

}  // namespace lps::avx512
