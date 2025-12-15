#pragma once

#include "lps/detail/convert.hpp"
#include "lps/detail/msb.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/doubling/basic_bit_mask.def.hpp"
#include "lps/doubling/basic_vector_mask.def.hpp"
#include "lps/doubling/vector.def.hpp"
#include "lps/generic/basic_vector_mask.hpp"
#include "lps/generic/vector.hpp"
#include "lps/stdint.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>

namespace lps::doubling {

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vector(const std::array<T, N>& src) :
      raw(std::bit_cast<std::array<half_vector, 2>>(src)) {
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zero() {
    vector<T, N, Env> result;
    result.raw = { half_vector::zero(), half_vector::zero() };
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::splat(T value) {
    vector<T, N, Env> result;
    result.raw = { half_vector::splat(value), half_vector::splat(value) };
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::splat(half_vector value) {
    vector<T, N, Env> result;
    result.raw = { value, value };
    return result;
  }

  template<class T, usize N, class Env>
  vector<T, N, Env> vector<T, N, Env>::load(const void* src) {
    vector v;
    std::memcpy(v.raw.data(), src, sizeof(v.raw));
    return v;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr T vector<T, N, Env>::read(usize i) const {
    T value;
    std::memcpy(&value, reinterpret_cast<const char*>(raw.data()) + i * sizeof(T), sizeof(T));
    return value;
  }

  template<class T, usize N, class Env>
  template<class U>
  LPS_INLINE constexpr detail::vector_clamped_size<Env, U, N> vector<T, N, Env>::convert() const {
    return detail::convert<Env, T, U, N>(*this);
  }

  template<class T, usize N, class Env>
  template<class V, usize extract_index>
  LPS_INLINE constexpr V vector<T, N, Env>::extract_aligned() const {
    V value;
    std::memcpy(&value, reinterpret_cast<const char*>(raw.data()) + extract_index * sizeof(V), sizeof(V));
    return value;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr std::tuple<typename vector<T, N, Env>::half_vector, typename vector<T, N, Env>::half_vector> vector<T, N, Env>::split() const {
    return { raw[0], raw[1] };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr typename vector<T, N, Env>::dup_vector vector<T, N, Env>::dup() const {
    return dup_vector::splat(*this);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::swizzle(const vector<T, N, Env>& src) const {
    vector<T, N, Env> result;
    result.raw[0] = raw[0].swizzle(src.raw[0], src.raw[1]);
    result.raw[1] = raw[1].swizzle(src.raw[0], src.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::swizzle(const vector<T, N, Env>& src0, const vector<T, N, Env>& src1) const {
    static_assert(std::has_single_bit(N));

    auto mask0 = test(vector<T, N, Env>::splat(N));
    auto index = andnot(vector<T, N, Env>::splat(N));
    return mask0.select(index.swizzle(src0), index.swizzle(src1));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::swizzle(const vmask_type& src) const {
    return std::bit_cast<vmask_type>(swizzle(std::bit_cast<vector>(src)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::swizzle(const Env::template vector<T, 16 / sizeof(T)>& src) const
    requires(16 / sizeof(T) != N)
  {
    vector<T, N, Env> result;
    result.raw[0] = raw[0].swizzle(src);
    result.raw[1] = raw[1].swizzle(src);
    return result;
  }

  template<class T, usize N, class Env>
  template<usize shift_amount>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::shl() const {
    vector<T, N, Env> result;
    result.raw[0] = raw[0].template shl<shift_amount>();
    result.raw[1] = raw[1].template shl<shift_amount>();
    return result;
  }

  template<class T, usize N, class Env>
  template<usize shift_amount>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::shr() const {
    vector<T, N, Env> result;
    result.raw[0] = raw[0].template shr<shift_amount>();
    result.raw[1] = raw[1].template shr<shift_amount>();
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::andnot(const vector<T, N, Env>& second) const {
    vector<T, N, Env> result;
    result.raw[0] = raw[0].andnot(second.raw[0]);
    result.raw[1] = raw[1].andnot(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr T vector<T, N, Env>::reduce_add() const {
    return (raw[0] + raw[1]).reduce_add();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr T vector<T, N, Env>::reduce_or() const {
    return (raw[0] | raw[1]).reduce_or();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr T vector<T, N, Env>::reduce_xor() const {
    return (raw[0] ^ raw[1]).reduce_xor();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zip_low(const vector<T, N, Env>& second) const {
    vector<T, N, Env> result;
    result.raw[0] = raw[0].zip_low(second.raw[0]);
    result.raw[1] = raw[0].zip_high(second.raw[0]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zip_high(const vector<T, N, Env>& second) const {
    vector<T, N, Env> result;
    result.raw[0] = raw[1].zip_low(second.raw[1]);
    result.raw[1] = raw[1].zip_high(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zip_low_128lanes(const vector<T, N, Env>& second) const {
    vector<T, N, Env> result;
    result.raw[0] = raw[0].zip_low_128lanes(second.raw[0]);
    result.raw[1] = raw[1].zip_low_128lanes(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zip_high_128lanes(const vector<T, N, Env>& second) const {
    vector<T, N, Env> result;
    result.raw[0] = raw[0].zip_high_128lanes(second.raw[0]);
    result.raw[1] = raw[1].zip_high_128lanes(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::test_vm(const vector& second) const {
    vector<T, N, Env>::mask_type result;
    result.raw[0] = raw[0].test_vm(second.raw[0]);
    result.raw[1] = raw[1].test_vm(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::test_bm(const vector& second) const {
    detail::bit_mask_base_t<N> result = 0;
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[0].test_bm(second.raw[0]).to_bits());
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[1].test_bm(second.raw[1]).to_bits()) << (N / 2);
    return bmask_type { result };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::test(const vector& second) const {
    if constexpr (Env::prefer_bm) {
      return test_bm(second);
    } else {
      return test_vm(second);
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::eq_vm(const vector& second) const {
    vector<T, N, Env>::mask_type result;
    result.raw[0] = raw[0].eq_vm(second.raw[0]);
    result.raw[1] = raw[1].eq_vm(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::eq_bm(const vector& second) const {
    detail::bit_mask_base_t<N> result = 0;
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[0].eq_bm(second.raw[0]).to_bits());
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[1].eq_bm(second.raw[1]).to_bits()) << (N / 2);
    return bmask_type { result };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::eq(const vector& second) const {
    if constexpr (Env::prefer_bm) {
      return eq_bm(second);
    } else {
      return eq_vm(second);
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::neq_vm(const vector& second) const {
    vector<T, N, Env>::mask_type result;
    result.raw[0] = raw[0].neq_vm(second.raw[0]);
    result.raw[1] = raw[1].neq_vm(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::neq_bm(const vector& second) const {
    detail::bit_mask_base_t<N> result = 0;
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[0].neq_bm(second.raw[0]).to_bits());
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[1].neq_bm(second.raw[1]).to_bits()) << (N / 2);
    return bmask_type { result };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::neq(const vector& second) const {
    if constexpr (Env::prefer_bm) {
      return neq_bm(second);
    } else {
      return neq_vm(second);
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::gt_vm(const vector& second) const {
    vector<T, N, Env>::mask_type result;
    result.raw[0] = raw[0].gt_vm(second.raw[0]);
    result.raw[1] = raw[1].gt_vm(second.raw[1]);
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::gt_bm(const vector& second) const {
    detail::bit_mask_base_t<N> result = 0;
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[0].gt_bm(second.raw[0]).to_bits());
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[1].gt_bm(second.raw[1]).to_bits()) << (N / 2);
    return bmask_type { result };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::gt(const vector& second) const {
    if constexpr (Env::prefer_bm) {
      return gt_bm(second);
    } else {
      return gt_vm(second);
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::nonzeros_vm() const {
    vector<T, N, Env>::mask_type result;
    result.raw[0] = raw[0].nonzeros_vm();
    result.raw[1] = raw[1].nonzeros_vm();
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::nonzeros_bm() const {
    detail::bit_mask_base_t<N> result = 0;
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[0].nonzeros_bm().to_bits());
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[1].nonzeros_bm().to_bits()) << (N / 2);
    return bmask_type { result };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::nonzeros() const {
    if constexpr (Env::prefer_bm) {
      return nonzeros_bm();
    } else {
      return nonzeros_vm();
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr usize vector<T, N, Env>::nonzeros_count() const {
    usize result = 0;
    result += raw[0].nonzeros_count();
    result += raw[1].nonzeros_count();
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::zeros_vm() const {
    vector<T, N, Env>::mask_type result;
    result.raw[0] = raw[0].zeros_vm();
    result.raw[1] = raw[1].zeros_vm();
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::zeros_bm() const {
    detail::bit_mask_base_t<N> result = 0;
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[0].zeros_bm().to_bits());
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[1].zeros_bm().to_bits()) << (N / 2);
    return bmask_type { result };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::zeros() const {
    if constexpr (Env::prefer_bm) {
      return zeros_bm();
    } else {
      return zeros_vm();
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr usize vector<T, N, Env>::zeros_count() const {
    usize result = 0;
    result += raw[0].zeros_count();
    result += raw[1].zeros_count();
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::msb_vm() const {
    vector<T, N, Env>::mask_type result;
    result.raw[0] = raw[0].msb_vm();
    result.raw[1] = raw[1].msb_vm();
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::msb_bm() const {
    detail::bit_mask_base_t<N> result = 0;
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[0].msb_bm().to_bits());
    result |= static_cast<detail::bit_mask_base_t<N>>(raw[1].msb_bm().to_bits()) << (N / 2);
    return bmask_type { result };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::msb() const {
    if constexpr (Env::prefer_bm) {
      return msb_bm();
    } else {
      return msb_vm();
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE std::array<T, N> vector<T, N, Env>::to_array() const {
    return std::bit_cast<std::array<T, N>>(raw);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr bool operator==(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first.raw == second.raw;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator~(const vector<T, N, Env>& first) {
    vector<T, N, Env> result;
    result.raw[0] = ~first.raw[0];
    result.raw[1] = ~first.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator&(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    vector<T, N, Env> result;
    result.raw[0] = first.raw[0] & second.raw[0];
    result.raw[1] = first.raw[1] & second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator&=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first & second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator|(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    vector<T, N, Env> result;
    result.raw[0] = first.raw[0] | second.raw[0];
    result.raw[1] = first.raw[1] | second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator|=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first | second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator^(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    vector<T, N, Env> result;
    result.raw[0] = first.raw[0] ^ second.raw[0];
    result.raw[1] = first.raw[1] ^ second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator^=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first ^ second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator+(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    vector<T, N, Env> result;
    result.raw[0] = first.raw[0] + second.raw[0];
    result.raw[1] = first.raw[1] + second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator+=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first + second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator-(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    vector<T, N, Env> result;
    result.raw[0] = first.raw[0] - second.raw[0];
    result.raw[1] = first.raw[1] - second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator-=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first - second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator*(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    vector<T, N, Env> result;
    result.raw[0] = first.raw[0] * second.raw[0];
    result.raw[1] = first.raw[1] * second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator*=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first * second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator<<(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    vector<T, N, Env> result;
    result.raw[0] = first.raw[0] << second.raw[0];
    result.raw[1] = first.raw[1] << second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator<<=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first << second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator>>(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    vector<T, N, Env> result;
    result.raw[0] = first.raw[0] >> second.raw[0];
    result.raw[1] = first.raw[1] >> second.raw[1];
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator>>=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first >> second;
  }

}  // namespace lps::doubling
