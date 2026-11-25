#pragma once

#include "lps2/arch.hpp"
#include "lps2/detail/vector_arch_type.hpp"
#include "lps2/detail/vector_clamped_size.hpp"
#include "lps2/lps2.fwd.hpp"
#include "lps2/stdint.hpp"

#include <array>
#include <bit>
#include <cassert>
#include <climits>
#include <concepts>
#include <type_traits>
#include <utility>

namespace lps2 {

  template<class T, usize N>
  struct vector {
    using element_type = T;
    inline static constexpr usize size = N;
    inline static constexpr usize bytesize = sizeof(T) * N;
    inline static constexpr usize bitsize = bytesize * CHAR_BIT;

    using raw_type = detail::vector_arch_type_t<T, N>;
    static_assert(sizeof(raw_type) == bytesize);

    using bmask_type = bit_mask<T, N>;
    using vmask_type = vector_mask<T, N>;

#if LPS2_PREFERS_BIT_MASK
    using mask_type = bit_mask<T, N>;
#endif
#if LPS2_PREFERS_VECTOR_MASK
    using mask_type = vector_mask<T, N>;
#endif

    LPS2_INLINE constexpr vector() = default;

    LPS2_INLINE constexpr vector(raw_type src) :
        raw(src) {
    }

    template<class... Args>
      requires(sizeof...(Args) == N) && (std::convertible_to<std::remove_cvref_t<Args>, T> && ...)
    LPS2_INLINE constexpr vector(Args&&... args) :
        raw(std::bit_cast<raw_type>(std::array<T, N> { static_cast<T>(args)... })) {
      assert((std::in_range<T>(args) && ...));
    }

    template<class... Args>
    LPS2_INLINE explicit constexpr vector(const std::array<T, N>& array) :
        raw(std::bit_cast<raw_type>(array)) {
    }

    [[nodiscard]] LPS2_INLINE constexpr std::array<T, N> to_array() const {
      return std::bit_cast<std::array<T, N>>(raw);
    }

    [[nodiscard]] LPS2_INLINE constexpr std::array<vector<T, N / 2>, 2> to_halves() const
      requires(N >= 2)
    {
#if LPS2_ARCH_AVX512BASE
      if constexpr (bitsize == 512) {
        return { _mm512_castsi512_si256(raw), _mm512_extracti32x8_epi32(raw, 1) };
      }
#endif
      if constexpr (detail::vector_arch_type<T, N>::is_doubled) {
        return { vector<T, N / 2> { raw[0] }, vector<T, N / 2> { raw[1] } };
      } else {
        return std::bit_cast<std::array<vector<T, N / 2>, 2>>(*this);
      }
    }

    LPS2_INLINE static constexpr vector concat(const vector<T, N / 2>& lower, const vector<T, N / 2>& upper) {
#if LPS2_ARCH_AVX512BASE
      if constexpr (bitsize == 512) {
        return { _mm512_shuffle_i32x4(_mm512_castsi256_si512(lower.raw), _mm512_castsi256_si512(upper.raw), 0b01000100) };
      }
#endif
      return std::bit_cast<vector>(std::array<vector<T, N / 2>, 2> { lower, upper });
    }

    static constexpr vector zero();
    static constexpr vector splat(T value);
    static constexpr vector load(const void* src);
    constexpr void store(void* dst) const;

    constexpr T read(usize i) const;

    template<class U>
    constexpr detail::vector_clamped_size<U, N> convert() const;

    template<class V, usize extract_index>
    constexpr V extract_aligned() const;

    // forall i: result[i] = src[this[i]]
    constexpr vector swizzle(const vector& src) const;
    // forall i: result[i] = concat[this[i]] where concat is concatenation of src0 and src1
    constexpr vector swizzle(const vector& src0, const vector& src1) const;

    constexpr vmask_type swizzle(const vmask_type& src) const;
    constexpr bmask_type swizzle(const bmask_type& src) const;

    constexpr vector swizzle(const vector<T, 16 / sizeof(T)>& src) const
      requires(16 / sizeof(T) != N);

    template<usize shift_amount>
    constexpr vector shl() const;
    template<usize shift_amount>
    constexpr vector shr() const;

    constexpr vector andnot(const vector& second) const;

    constexpr vector clamp(const vector& min, const vector& max) const;

    template<class U>
      requires(sizeof(T) == sizeof(U))
    constexpr auto pair_dot(const vector<U, N>& second) const;

    template<class V1, class V2>
    constexpr vector accumulate_pair_dot(const V1& first, const V2& second) const;

    constexpr T reduce_add() const;
    constexpr T reduce_or() const;
    constexpr T reduce_xor() const;

    constexpr vector zip_low(const vector& second) const;
    constexpr vector zip_high(const vector& second) const;

    constexpr vector zip_low_128bit(const vector& second) const;
    constexpr vector zip_high_128bit(const vector& second) const;

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

    raw_type raw;
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

}  // namespace lps2
