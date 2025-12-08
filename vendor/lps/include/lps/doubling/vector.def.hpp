#pragma once

#include "lps/detail/bit_mask_base.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/doubling/doubling.fwd.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <tuple>

namespace lps::doubling {

  template<class T, usize N, class Env>
  struct vector {
    using element_type = T;
    static constexpr usize size = N;

    using bmask_type = typename Env::template bit_mask<T, N>;
    using vmask_type = typename Env::template vector_mask<T, N>;
    using mask_type = typename Env::template mask<T, N>;

    using half_vector = typename Env::template vector<T, N / 2>;
    using dup_vector = typename Env::template vector<T, N * 2>;

    constexpr vector() = default;

    explicit constexpr vector(const std::array<T, N>& src);

    static constexpr vector zero();
    static constexpr vector splat(T value);
    static constexpr vector splat(half_vector value);
    static vector load(const void* src);

    constexpr T read(usize i) const;

    template<class U>
    constexpr detail::vector_clamped_size<Env, U, N> convert() const;

    template<class V, usize extract_index>
    constexpr V extract_aligned() const;
    constexpr std::tuple<half_vector, half_vector> split() const;
    constexpr dup_vector dup() const;

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
    constexpr bmask_type eq_bm(const vector& second) const;
    constexpr mask_type eq(const vector& second) const;

    constexpr vmask_type neq_vm(const vector& second) const;
    constexpr bmask_type neq_bm(const vector& second) const;
    constexpr mask_type neq(const vector& second) const;

    constexpr vmask_type gt_vm(const vector& second) const;
    constexpr bmask_type gt_bm(const vector& second) const;
    constexpr mask_type gt(const vector& second) const;

    constexpr vmask_type nonzeros_vm() const;
    constexpr bmask_type nonzeros_bm() const;
    constexpr mask_type nonzeros() const;
    constexpr usize nonzeros_count() const;

    constexpr vmask_type zeros_vm() const;
    constexpr bmask_type zeros_bm() const;
    constexpr mask_type zeros() const;
    constexpr usize zeros_count() const;

    constexpr vmask_type msb_vm() const;
    constexpr bmask_type msb_bm() const;
    constexpr mask_type msb() const;

    [[nodiscard]] std::array<T, N> to_array() const;

    std::array<half_vector, 2> raw;
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

}  // namespace lps::doubling
