#pragma once

#include "lps/avx2/avx2.fwd.hpp"
#include "lps/detail/bit_mask_base.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <immintrin.h>
#include <tuple>

namespace lps::avx2 {

  template<class T, usize N, class Env>
  struct vector {
    static constexpr bool is_128_bit = N * sizeof(T) == 16 * sizeof(u8);
    static constexpr bool is_256_bit = N * sizeof(T) == 32 * sizeof(u8);
    static_assert(is_128_bit || is_256_bit);
    using raw_type = std::conditional_t<is_128_bit, __m128i, __m256i>;

    using element_type = T;
    static constexpr usize size = N;

    using bmask_type = typename detail::bit_mask_base_t<N>;
    using vmask_type = typename Env::template vector_mask<T, N>;
    using mask_type = typename Env::template vector_mask<T, N>;

    constexpr vector() = default;
    explicit constexpr vector(raw_type src);
    explicit constexpr vector(const std::array<T, N>& src);

    static constexpr vector zero();
    static constexpr vector splat(T value);
    static vector load(const void* src);

    constexpr T read(usize i) const;

    template<class U>
    constexpr detail::vector_clamped_size<Env, U, N> convert() const;

    template<class V, usize extract_index>
    constexpr V extract_aligned() const;

    // forall i: result[i] = src[this[i]]
    constexpr vector swizzle(const vector& src) const;
    // forall i: result[i] = concat[this[i]] where contact is concatenation of src0 and src1
    constexpr vector swizzle(const vector& src0, const vector& src1) const;

    constexpr vmask_type swizzle(const vmask_type& src) const;

    constexpr vector swizzle(const Env::template vector<T, 16 / sizeof(T)>& src) const
      requires(16 / sizeof(T) != N);

    template<usize shift_amount>
    constexpr vector shl() const;
    template<usize shift_amount>
    constexpr vector shr() const;

    constexpr vector andnot(const vector& second) const;

    constexpr T reduce_add() const;
    constexpr T reduce_or() const;
    constexpr T reduce_xor() const;

    constexpr vector zip_low(const vector& second) const;
    constexpr vector zip_high(const vector& second) const;

    constexpr vector zip_low_128lanes(const vector& second) const;
    constexpr vector zip_high_128lanes(const vector& second) const;

    constexpr vmask_type test_vm(const vector& second) const;
    constexpr bmask_type test_bm(const vector& second) const;
    constexpr mask_type test(const vector& second) const;

    constexpr vmask_type eq_vm(const vector& second) const;
    constexpr mask_type eq(const vector& second) const;

    constexpr vmask_type neq_vm(const vector& second) const;
    constexpr mask_type neq(const vector& second) const;

    constexpr vmask_type gt_vm(const vector& second) const;
    constexpr mask_type gt(const vector& second) const;

    constexpr vmask_type nonzeros_vm() const;
    constexpr mask_type nonzeros() const;
    constexpr usize nonzeros_count() const;

    constexpr vmask_type zeros_vm() const;
    constexpr mask_type zeros() const;
    constexpr usize zeros_count() const;

    constexpr vmask_type msb_vm() const;
    constexpr mask_type msb() const;

    [[nodiscard]] constexpr std::array<T, N> to_array() const;

    raw_type raw;
  };

  template<class T, usize N, class Env>
  constexpr bool operator==(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator~(const vector<T, N, Env>& first);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator&(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator&=(vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator|(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator|=(vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator^(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator^=(vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator+(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator+=(vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator-(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator-=(vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator*(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator*=(vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator<<(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator<<=(vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator>>(const vector<T, N, Env>& first, const vector<T, N, Env>& second);

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator>>=(vector<T, N, Env>& first, const vector<T, N, Env>& second);

}  // namespace lps::avx2
