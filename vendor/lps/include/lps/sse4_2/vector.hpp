#pragma once

#include "lps/detail/always_false.hpp"
#include "lps/detail/convert.hpp"
#include "lps/detail/msb.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/generic/basic_vector_mask.def.hpp"
#include "lps/generic/vector.def.hpp"
#include "lps/sse4_2/basic_vector_mask.def.hpp"
#include "lps/sse4_2/vector.def.hpp"
#include "lps/stdint.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <type_traits>

namespace lps::sse4_2 {

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vector(__m128i src) :
      raw(src) {
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vector(const std::array<T, N>& src) :
      raw(std::bit_cast<__m128i>(src)) {
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::zero() {
    vector result;
    result.raw = _mm_setzero_si128();
    return result;
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::splat(T value) {
    vector result;
    if constexpr (sizeof(T) == sizeof(u8)) {
      result.raw = _mm_set1_epi8(std::bit_cast<i8>(value));
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      result.raw = _mm_set1_epi16(std::bit_cast<i16>(value));
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      result.raw = _mm_set1_epi32(std::bit_cast<i32>(value));
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      result.raw = _mm_set1_epi64x(std::bit_cast<i64>(value));
    } else {
      static_assert(detail::always_false<T>);
    }
    return result;
  }

  template<class T, usize N, class Env>
  vector<T, N, Env> vector<T, N, Env>::load(const void* src) {
    vector result;
    result.raw = _mm_loadu_si128(reinterpret_cast<__m128i const*>(src));
    return result;
  }

  template<class T, usize N, class Env>
  constexpr T vector<T, N, Env>::read(usize i) const {
    T value;
    std::memcpy(&value, reinterpret_cast<const char*>(&raw) + i * sizeof(T), sizeof(T));
    return value;
  }

  template<class T, usize N, class Env>
  template<class U>
  constexpr detail::vector_clamped_size<Env, U, N> vector<T, N, Env>::convert() const {
    return detail::convert<Env, T, U, N>(*this);
  }

  template<class T, usize N, class Env>
  template<class V, usize extract_index>
  constexpr V vector<T, N, Env>::extract_aligned() const {
    V value;
    std::memcpy(&value, reinterpret_cast<const char*>(&raw) + extract_index * sizeof(V), sizeof(V));
    return value;
  }

  template<class T, usize N, class Env>
  constexpr typename vector<T, N, Env>::dup_vector vector<T, N, Env>::dup() const {
    return dup_vector::splat(*this);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::swizzle(const vector<T, N, Env>& src) const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      return vector { _mm_shuffle_epi8(src.raw, raw) };
    } else {
      return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(*this).swizzle(std::bit_cast<generic::vector<T, N>>(src)));
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::swizzle(const vector<T, N, Env>& src0, const vector<T, N, Env>& src1) const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      __m128i mask = _mm_slli_epi16(raw, 3);
      return vector { _mm_blendv_epi8(_mm_shuffle_epi8(src0.raw, raw), _mm_shuffle_epi8(src1.raw, raw), mask) };
    } else {
      return std::bit_cast<vector<T, N, Env>>(
        std::bit_cast<generic::vector<T, N>>(*this).swizzle(std::bit_cast<generic::vector<T, N>>(src0), std::bit_cast<generic::vector<T, N>>(src1)));
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::swizzle(const mask_type& src) const {
    return std::bit_cast<mask_type>(std::bit_cast<generic::vector<T, N>>(*this).swizzle(std::bit_cast<generic::vector_mask<T, N>>(src)));
  }

  template<class T, usize N, class Env>
  template<usize shift_amount>
  constexpr vector<T, N, Env> vector<T, N, Env>::shl() const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      constexpr u8 mask = static_cast<u8>(0xFF << shift_amount);
      return vector { _mm_slli_epi16(raw, shift_amount) } & vector::splat(mask);
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return vector { _mm_slli_epi16(raw, shift_amount) };
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return vector { _mm_slli_epi32(raw, shift_amount) };
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return vector { _mm_slli_epi64(raw, shift_amount) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  template<usize shift_amount>
  constexpr vector<T, N, Env> vector<T, N, Env>::shr() const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      constexpr u8 mask = static_cast<u8>(0xFF >> shift_amount);
      return vector { _mm_srli_epi16(raw, shift_amount) } & vector::splat(mask);
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return vector { _mm_srli_epi16(raw, shift_amount) };
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return vector { _mm_srli_epi32(raw, shift_amount) };
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return vector { _mm_srli_epi64(raw, shift_amount) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::andnot(const vector<T, N, Env>& second) const {
    return vector { _mm_andnot_si128(second.raw, raw) };
  }

  template<class T, usize N, class Env>
  constexpr T vector<T, N, Env>::reduce_add() const {
    return std::bit_cast<generic::vector<T, N>>(*this).reduce_add();
  }

  template<class T, usize N, class Env>
  constexpr T vector<T, N, Env>::reduce_or() const {
    return std::bit_cast<generic::vector<T, N>>(*this).reduce_or();
  }

  template<class T, usize N, class Env>
  constexpr T vector<T, N, Env>::reduce_xor() const {
    return std::bit_cast<generic::vector<T, N>>(*this).reduce_xor();
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::zip_low(const vector<T, N, Env>& second) const {
    // SAFETY: We are made up of one 128 lane.
    return zip_low_128lanes(second);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::zip_high(const vector<T, N, Env>& second) const {
    // SAFETY: We are made up of one 128 lane.
    return zip_high_128lanes(second);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::zip_low_128lanes(const vector<T, N, Env>& second) const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      return vector { _mm_unpacklo_epi8(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return vector { _mm_unpacklo_epi16(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return vector { _mm_unpacklo_epi32(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return vector { _mm_unpacklo_epi64(raw, second.raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> vector<T, N, Env>::zip_high_128lanes(const vector<T, N, Env>& second) const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      return vector { _mm_unpackhi_epi8(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return vector { _mm_unpackhi_epi16(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return vector { _mm_unpackhi_epi32(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return vector { _mm_unpackhi_epi64(raw, second.raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::test_vm(const vector& second) const {
    return zero().neq_vm(*this & second);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::test_bm(const vector& second) const {
    return ~zero().eq_vm(*this & second).to_bits();
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::test(const vector& second) const {
    return test_vm(second);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::eq_vm(const vector& second) const {
    if constexpr (sizeof(T) == sizeof(u8)) {
      return mask_type { _mm_cmpeq_epi8(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return mask_type { _mm_cmpeq_epi16(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return mask_type { _mm_cmpeq_epi32(raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return mask_type { _mm_cmpeq_epi64(raw, second.raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::eq(const vector& second) const {
    return eq_vm(second);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::neq_vm(const vector& second) const {
    return ~eq_vm(second);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::neq(const vector& second) const {
    return neq_vm(second);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::gt_vm(const vector& second) const {
    if constexpr (std::is_same_v<T, i8>) {
      return mask_type { _mm_cmpgt_epi8(raw, second.raw) };
    } else if constexpr (std::is_same_v<T, i16>) {
      return mask_type { _mm_cmpgt_epi16(raw, second.raw) };
    } else if constexpr (std::is_same_v<T, i32>) {
      return mask_type { _mm_cmpgt_epi32(raw, second.raw) };
    } else if constexpr (std::is_same_v<T, i64>) {
      return mask_type { _mm_cmpgt_epi64(raw, second.raw) };
    } else if constexpr (std::is_same_v<T, u8>) {
      return ~mask_type { _mm_cmpeq_epi8(_mm_min_epu8(raw, second.raw), raw) };
    } else if constexpr (std::is_same_v<T, u16>) {
      return ~mask_type { _mm_cmpeq_epi16(_mm_min_epu16(raw, second.raw), raw) };
    } else if constexpr (std::is_same_v<T, u32>) {
      return ~mask_type { _mm_cmpeq_epi32(_mm_min_epu32(raw, second.raw), raw) };
    } else if constexpr (std::is_same_v<T, u64>) {
      static_assert(detail::always_false<T>, "unimplemented");
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::gt(const vector& other) const {
    return gt_vm(other);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::nonzeros_vm() const {
    return neq_vm(zero());
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::nonzeros() const {
    return nonzeros_vm();
  }

  template<class T, usize N, class Env>
  constexpr usize vector<T, N, Env>::nonzeros_count() const {
    return nonzeros().popcount();
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::zeros_vm() const {
    return eq_vm(zero());
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::zeros() const {
    return zeros_vm();
  }

  template<class T, usize N, class Env>
  constexpr usize vector<T, N, Env>::zeros_count() const {
    return zeros().popcount();
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::msb_vm() const {
    constexpr T msb_bit = static_cast<T>(1) << (sizeof(T) * CHAR_BIT - 1);
    return test_vm(vector::splat(msb_bit));
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::msb() const {
    return msb_vm();
  }

  template<class T, usize N, class Env>
  std::array<T, N> vector<T, N, Env>::to_array() const {
    return std::bit_cast<std::array<T, N>>(raw);
  }

  template<class T, usize N, class Env>
  constexpr bool operator==(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    __m128i x = _mm_xor_si128(first.raw, second.raw);
    return _mm_testz_si128(x, x);
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator~(const vector<T, N, Env>& first) {
    return first ^ vector<T, N, Env>::splat(static_cast<T>(~static_cast<T>(0)));
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator&(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return vector<T, N, Env> { _mm_and_si128(first.raw, second.raw) };
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator&=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first & second;
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator|(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return vector<T, N, Env> { _mm_or_si128(first.raw, second.raw) };
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator|=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first | second;
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator^(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return vector<T, N, Env> { _mm_xor_si128(first.raw, second.raw) };
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator^=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first ^ second;
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator+(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (sizeof(T) == sizeof(u8)) {
      return vector<T, N, Env> { _mm_add_epi8(first.raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return vector<T, N, Env> { _mm_add_epi16(first.raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return vector<T, N, Env> { _mm_add_epi32(first.raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return vector<T, N, Env> { _mm_add_epi64(first.raw, second.raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator+=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first + second;
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator-(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (sizeof(T) == sizeof(u8)) {
      return vector<T, N, Env> { _mm_sub_epi8(first.raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u16)) {
      return vector<T, N, Env> { _mm_sub_epi16(first.raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return vector<T, N, Env> { _mm_sub_epi32(first.raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u64)) {
      return vector<T, N, Env> { _mm_sub_epi64(first.raw, second.raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator-=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first - second;
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator*(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (sizeof(T) == sizeof(u16)) {
      return vector<T, N, Env> { _mm_mullo_epi16(first.raw, second.raw) };
    } else if constexpr (sizeof(T) == sizeof(u32)) {
      return vector<T, N, Env> { _mm_mullo_epi32(first.raw, second.raw) };
    } else {
      return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(first) * std::bit_cast<generic::vector<T, N>>(second));
    }
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator*=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first * second;
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator<<(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(first) << std::bit_cast<generic::vector<T, N>>(second));
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator<<=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first << second;
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env> operator>>(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(first) >> std::bit_cast<generic::vector<T, N>>(second));
  }

  template<class T, usize N, class Env>
  constexpr vector<T, N, Env>& operator>>=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first >> second;
  }

}  // namespace lps::sse4_2
