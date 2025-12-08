#pragma once

#include "lps/detail/bit_mask_base.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/generic/generic.fwd.hpp"
#include "lps/stdint.hpp"

#include <array>

namespace lps::generic {

  template<class T, usize N>
  struct vector {
    using element_type = T;
    static constexpr usize size = N;

    using vmask_type = vector_mask<T, N>;
    using mask_type = vector_mask<T, N>;

    using bmask_type = typename detail::bit_mask_base_t<N>;
    using half_vector = vector<T, N / 2>;
    using dup_vector = vector<T, N * 2>;

    constexpr vector() = default;

    explicit constexpr vector(const std::array<T, N>& src) :
        raw(src) {
    }

    static constexpr vector zero();
    static constexpr vector splat(T value);
    static constexpr vector splat(half_vector value);
    static vector load(const void* src);

    constexpr T read(usize i) const;

    template<class U>
    constexpr vector<U, detail::clamped_size<U, N>> convert() const;

    template<class V, usize extract_index>
    constexpr V extract_aligned() const;
    constexpr std::tuple<half_vector, half_vector> split() const;
    constexpr dup_vector dup() const;

    // forall i: result[i] = src[this[i]]
    constexpr vector swizzle(const vector& src) const;
    // forall i: result[i] = concat[this[i]] where contact is concatenation of src0 and src1
    constexpr vector swizzle(const vector& src0, const vector& src1) const;

    constexpr mask_type swizzle(const mask_type& src) const;

    template<usize M>
      requires(M != N)
    constexpr vector swizzle(const vector<T, M>& src) const;

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

    [[nodiscard]] std::array<T, N> to_array() const;

    std::array<T, N> raw;
  };

  template<class T, usize N>
  constexpr bool operator==(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N> operator~(const vector<T, N>& first);

  template<class T, usize N>
  constexpr vector<T, N> operator&(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N>& operator&=(vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N> operator|(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N>& operator|=(vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N> operator^(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N>& operator^=(vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N> operator+(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N>& operator+=(vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N> operator-(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N>& operator-=(vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N> operator*(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N>& operator*=(vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N> operator<<(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N>& operator<<=(vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N> operator>>(const vector<T, N>& first, const vector<T, N>& second);

  template<class T, usize N>
  constexpr vector<T, N>& operator>>=(vector<T, N>& first, const vector<T, N>& second);

}  // namespace lps::generic
