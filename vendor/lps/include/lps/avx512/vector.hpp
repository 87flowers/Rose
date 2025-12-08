#pragma once

#include "lps/avx512/basic_vector_mask.def.hpp"
#include "lps/avx512/vector.def.hpp"
#include "lps/detail/always_false.hpp"
#include "lps/detail/msb.hpp"
#include "lps/detail/vector_clamped_size.hpp"
#include "lps/generic/basic_vector_mask.def.hpp"
#include "lps/generic/vector.def.hpp"
#include "lps/stdint.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <type_traits>

namespace lps::avx512 {

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vector(raw_type src) :
      raw(src) {
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vector(const std::array<T, N>& src) :
      raw(std::bit_cast<raw_type>(src)) {
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zero() {
    vector result;
    if constexpr (is_128_bit) {
      result.raw = _mm_setzero_si128();
    } else if constexpr (is_256_bit) {
      result.raw = _mm256_setzero_si256();
    } else {
      result.raw = _mm512_setzero_si512();
    }
    return result;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::splat(T value) {
    vector result;
    if constexpr (is_128_bit) {
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
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        result.raw = _mm256_set1_epi8(std::bit_cast<i8>(value));
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        result.raw = _mm256_set1_epi16(std::bit_cast<i16>(value));
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        result.raw = _mm256_set1_epi32(std::bit_cast<i32>(value));
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        result.raw = _mm256_set1_epi64x(std::bit_cast<i64>(value));
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        result.raw = _mm512_set1_epi8(std::bit_cast<i8>(value));
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        result.raw = _mm512_set1_epi16(std::bit_cast<i16>(value));
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        result.raw = _mm512_set1_epi32(std::bit_cast<i32>(value));
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        result.raw = _mm512_set1_epi64(std::bit_cast<i64>(value));
      } else {
        static_assert(detail::always_false<T>);
      }
    }
    return result;
  }

  template<class T, usize N, class Env>
  vector<T, N, Env> vector<T, N, Env>::load(const void* src) {
    vector v;
    std::memcpy(&v.raw, src, sizeof(v.raw));
    return v;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr T vector<T, N, Env>::read(usize i) const {
    T value;
    std::memcpy(&value, reinterpret_cast<const char*>(&raw) + i * sizeof(T), sizeof(T));
    return value;
  }

  template<class T, usize N, class Env>
  template<class U>
  LPS_INLINE constexpr detail::vector_clamped_size<Env, U, N> vector<T, N, Env>::convert() const {
    // TOOD: Reimplement for AVX512
    using Result = detail::vector_clamped_size<Env, U, N>;
    if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        if constexpr (sizeof(U) == sizeof(u8)) {
          return *this;
        } else if constexpr (std::is_same_v<T, i8> && std::is_same_v<U, i16>) {
          return Result { _mm_cvtepi8_epi16(raw) };
        } else if constexpr (sizeof(U) == sizeof(u16)) {
          return Result { _mm_cvtepu8_epi16(raw) };
        } else if constexpr (std::is_same_v<T, i8> && std::is_same_v<U, i32>) {
          return Result { _mm_cvtepi8_epi32(raw) };
        } else if constexpr (sizeof(U) == sizeof(u32)) {
          return Result { _mm_cvtepu8_epi32(raw) };
        } else if constexpr (std::is_same_v<T, i8> && std::is_same_v<U, i64>) {
          return Result { _mm_cvtepi8_epi64(raw) };
        } else if constexpr (sizeof(U) == sizeof(u64)) {
          return Result { _mm_cvtepu8_epi64(raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        if constexpr (sizeof(U) == sizeof(u8)) {
          // TODO: Optimize
          return Result { _mm_packus_epi16(_mm_and_si128(raw, _mm_set1_epi16(0x00FF)), _mm_setzero_si128()) };
        } else if constexpr (sizeof(U) == sizeof(u16)) {
          return *this;
        } else if constexpr (std::is_same_v<T, i16> && std::is_same_v<U, i32>) {
          return Result { _mm_cvtepi16_epi32(raw) };
        } else if constexpr (sizeof(U) == sizeof(u32)) {
          return Result { _mm_cvtepu16_epi32(raw) };
        } else if constexpr (std::is_same_v<T, i16> && std::is_same_v<U, i64>) {
          return Result { _mm_cvtepi16_epi64(raw) };
        } else if constexpr (sizeof(U) == sizeof(u64)) {
          return Result { _mm_cvtepu16_epi64(raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        if constexpr (sizeof(U) == sizeof(u8)) {
          // TODO: Optimize
          return Result { _mm_packus_epi16(_mm_packus_epi32(_mm_and_si128(raw, _mm_set1_epi32(0x00FF)), _mm_setzero_si128()), _mm_setzero_si128()) };
        } else if constexpr (sizeof(U) == sizeof(u16)) {
          // TODO: Optimize
          return Result { _mm_packus_epi32(_mm_and_si128(raw, _mm_set1_epi32(0x0000FFFF)), _mm_setzero_si128()) };
        } else if constexpr (sizeof(U) == sizeof(u32)) {
          return *this;
        } else if constexpr (std::is_same_v<T, i32> && std::is_same_v<U, i64>) {
          return Result { _mm_cvtepi32_epi64(raw) };
        } else if constexpr (sizeof(U) == sizeof(u64)) {
          return Result { _mm_cvtepu32_epi64(raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        if constexpr (sizeof(U) == sizeof(u64)) {
          return *this;
        }
        // TODO: Implement rest
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        if constexpr (sizeof(U) == sizeof(u8)) {
          return *this;
        } else if constexpr (std::is_same_v<T, i8> && std::is_same_v<U, i16>) {
          return Result { _mm256_cvtepi8_epi16(raw) };
        } else if constexpr (sizeof(U) == sizeof(u16)) {
          return Result { _mm256_cvtepu8_epi16(raw) };
        } else if constexpr (std::is_same_v<T, i8> && std::is_same_v<U, i32>) {
          return Result { _mm256_cvtepi8_epi32(raw) };
        } else if constexpr (sizeof(U) == sizeof(u32)) {
          return Result { _mm256_cvtepu8_epi32(raw) };
        } else if constexpr (std::is_same_v<T, i8> && std::is_same_v<U, i64>) {
          return Result { _mm256_cvtepi8_epi64(raw) };
        } else if constexpr (sizeof(U) == sizeof(u64)) {
          return Result { _mm256_cvtepu8_epi64(raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        if constexpr (sizeof(U) == sizeof(u8)) {
          // TODO: Optimize
          return Result { _mm256_packus_epi16(_mm256_and_si256(raw, _mm256_set1_epi16(0x00FF)), _mm256_setzero_si256()) };
        } else if constexpr (sizeof(U) == sizeof(u16)) {
          return *this;
        } else if constexpr (std::is_same_v<T, i16> && std::is_same_v<U, i32>) {
          return Result { _mm256_cvtepi16_epi32(raw) };
        } else if constexpr (sizeof(U) == sizeof(u32)) {
          return Result { _mm256_cvtepu16_epi32(raw) };
        } else if constexpr (std::is_same_v<T, i16> && std::is_same_v<U, i64>) {
          return Result { _mm256_cvtepi16_epi64(raw) };
        } else if constexpr (sizeof(U) == sizeof(u64)) {
          return Result { _mm256_cvtepu16_epi64(raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        if constexpr (sizeof(U) == sizeof(u8)) {
          // TODO: Optimize
          return Result { _mm256_packus_epi16(_mm256_packus_epi32(_mm256_and_si256(raw, _mm256_set1_epi32(0x00FF)), _mm256_setzero_si256()),
                                              _mm256_setzero_si256()) };
        } else if constexpr (sizeof(U) == sizeof(u16)) {
          // TODO: Optimize
          return Result { _mm256_packus_epi32(_mm256_and_si256(raw, _mm256_set1_epi32(0x0000FFFF)), _mm256_setzero_si256()) };
        } else if constexpr (sizeof(U) == sizeof(u32)) {
          return *this;
        } else if constexpr (std::is_same_v<T, i32> && std::is_same_v<U, i64>) {
          return Result { _mm256_cvtepi32_epi64(raw) };
        } else if constexpr (sizeof(U) == sizeof(u64)) {
          return Result { _mm256_cvtepu32_epi64(raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        if constexpr (sizeof(U) == sizeof(u64)) {
          return *this;
        }
        // TODO: Implement rest
      } else {
        static_assert(detail::always_false<T>);
      }
    }

    std::array<U, std::max(N, 16 / sizeof(U))> result;
    for (usize i = 0; i < N; i++) {
      result[i] = static_cast<U>(read(i));
    }
    return Result { result };
  }

  template<class T, usize N, class Env>
  template<class V, usize extract_index>
  LPS_INLINE constexpr V vector<T, N, Env>::extract_aligned() const {
    V value;
    std::memcpy(&value, reinterpret_cast<const char*>(&raw) + extract_index * sizeof(V), sizeof(V));
    return value;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::swizzle(const vector<T, N, Env>& src) const {
    if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm_maskz_permutexvar_epi8(_knot_mask16(_mm_movepi8_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm_maskz_permutexvar_epi16(_knot_mask8(_mm_movepi16_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm_maskz_permutexvar_epi32(_knot_mask8(_mm_movepi32_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm_maskz_permutexvar_epi64(_knot_mask8(_mm_movepi64_mask(raw)), raw, src.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm256_maskz_permutexvar_epi8(_knot_mask32(_mm256_movepi8_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm256_maskz_permutexvar_epi16(_knot_mask16(_mm256_movepi16_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm256_maskz_permutexvar_epi32(_knot_mask8(_mm256_movepi32_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm256_maskz_permutexvar_epi64(_knot_mask8(_mm256_movepi64_mask(raw)), raw, src.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm512_maskz_permutexvar_epi8(_knot_mask64(_mm512_movepi8_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm512_maskz_permutexvar_epi16(_knot_mask32(_mm512_movepi16_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm512_maskz_permutexvar_epi32(_knot_mask16(_mm512_movepi32_mask(raw)), raw, src.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm512_maskz_permutexvar_epi64(_knot_mask8(_mm512_movepi64_mask(raw)), raw, src.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::swizzle(const vector<T, N, Env>& src0, const vector<T, N, Env>& src1) const {
    if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm_maskz_permutex2var_epi8(_knot_mask16(_mm_movepi8_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm_maskz_permutex2var_epi16(_knot_mask8(_mm_movepi16_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm_maskz_permutex2var_epi32(_knot_mask8(_mm_movepi32_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm_maskz_permutex2var_epi64(_knot_mask8(_mm_movepi64_mask(raw)), raw, src0.raw, src1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm256_maskz_permutex2var_epi8(_knot_mask32(_mm256_movepi8_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm256_maskz_permutex2var_epi16(_knot_mask16(_mm256_movepi16_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm256_maskz_permutex2var_epi32(_knot_mask8(_mm256_movepi32_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm256_maskz_permutex2var_epi64(_knot_mask8(_mm256_movepi64_mask(raw)), raw, src0.raw, src1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return { _mm512_maskz_permutex2var_epi8(_knot_mask64(_mm512_movepi8_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return { _mm512_maskz_permutex2var_epi16(_knot_mask32(_mm512_movepi16_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return { _mm512_maskz_permutex2var_epi32(_knot_mask16(_mm512_movepi32_mask(raw)), raw, src0.raw, src1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return { _mm512_maskz_permutex2var_epi64(_knot_mask8(_mm512_movepi64_mask(raw)), raw, src0.raw, src1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::swizzle(const vmask_type& src) const {
    return mask_type { swizzle(src.raw).raw };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::swizzle(const bmask_type& src) const {
    return mask_type { swizzle(src.to_vector()).msb().to_bits() };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::swizzle(const Env::template vector<T, 16 / sizeof(T)>& src) const
    requires(16 / sizeof(T) != N)
  {
    if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(src.raw), raw) };
      } else {
        return swizzle(vector<T, N, Env> { _mm256_broadcastsi128_si256(src.raw) });
      }
    } else if constexpr (is_512_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm512_shuffle_epi8(_mm512_broadcast_i32x4(src.raw), raw) };
      } else {
        return swizzle(vector<T, N, Env> { _mm512_broadcast_i32x4(src.raw) });
      }
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  template<usize shift_amount>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::shl() const {
    if constexpr (is_128_bit) {
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
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        constexpr u8 mask = static_cast<u8>(0xFF << shift_amount);
        return vector { _mm256_slli_epi16(raw, shift_amount) } & vector::splat(mask);
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm256_slli_epi16(raw, shift_amount) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm256_slli_epi32(raw, shift_amount) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm256_slli_epi64(raw, shift_amount) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        constexpr u8 mask = static_cast<u8>(0xFF << shift_amount);
        return vector { _mm512_slli_epi16(raw, shift_amount) } & vector::splat(mask);
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm512_slli_epi16(raw, shift_amount) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm512_slli_epi32(raw, shift_amount) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm512_slli_epi64(raw, shift_amount) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  template<usize shift_amount>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::shr() const {
    if constexpr (is_128_bit) {
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
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        constexpr u8 mask = static_cast<u8>(0xFF >> shift_amount);
        return vector { _mm256_srli_epi16(raw, shift_amount) } & vector::splat(mask);
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm256_srli_epi16(raw, shift_amount) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm256_srli_epi32(raw, shift_amount) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm256_srli_epi64(raw, shift_amount) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        constexpr u8 mask = static_cast<u8>(0xFF >> shift_amount);
        return vector { _mm512_srli_epi16(raw, shift_amount) } & vector::splat(mask);
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm512_srli_epi16(raw, shift_amount) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm512_srli_epi32(raw, shift_amount) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm512_srli_epi64(raw, shift_amount) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::andnot(const vector<T, N, Env>& second) const {
    if constexpr (is_128_bit) {
      return vector { _mm_andnot_si128(second.raw, raw) };
    } else if constexpr (is_256_bit) {
      return vector { _mm256_andnot_si256(second.raw, raw) };
    } else {
      return vector { _mm512_andnot_si512(second.raw, raw) };
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr T vector<T, N, Env>::reduce_add() const {
    return std::bit_cast<generic::vector<T, N>>(*this).reduce_add();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr T vector<T, N, Env>::reduce_or() const {
    if constexpr (sizeof(T) == sizeof(u64)) {
      __m128i r;
      if constexpr (is_128_bit) {
        r = raw;
      } else if constexpr (is_256_bit) {
        r = _mm_or_si128(_mm256_extracti128_si256(raw, 1), _mm256_castsi256_si128(raw));
      } else {
        __m256i r2 = _mm256_or_si256(_mm512_extracti64x4_epi64(raw, 1), _mm512_castsi512_si256(raw));
        r = _mm_or_si128(_mm256_extracti128_si256(r2, 1), _mm256_castsi256_si128(r2));
      }
      r = _mm_or_si128(r, _mm_unpackhi_epi64(r, r));
      return static_cast<u64>(_mm_extract_epi64(r, 0));
    }
    return std::bit_cast<generic::vector<T, N>>(*this).reduce_or();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr T vector<T, N, Env>::reduce_xor() const {
    return std::bit_cast<generic::vector<T, N>>(*this).reduce_xor();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zip_low(const vector<T, N, Env>& second) const {
    return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(*this).zip_low(std::bit_cast<generic::vector<T, N>>(second)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zip_high(const vector<T, N, Env>& second) const {
    return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(*this).zip_high(std::bit_cast<generic::vector<T, N>>(second)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zip_low_128lanes(const vector<T, N, Env>& second) const {
    if constexpr (is_128_bit) {
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
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm256_unpacklo_epi8(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm256_unpacklo_epi16(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm256_unpacklo_epi32(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm256_unpacklo_epi64(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm512_unpacklo_epi8(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm512_unpacklo_epi16(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm512_unpacklo_epi32(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm512_unpacklo_epi64(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> vector<T, N, Env>::zip_high_128lanes(const vector<T, N, Env>& second) const {
    if constexpr (is_128_bit) {
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
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm256_unpackhi_epi8(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm256_unpackhi_epi16(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm256_unpackhi_epi32(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm256_unpackhi_epi64(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector { _mm512_unpackhi_epi8(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector { _mm512_unpackhi_epi16(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector { _mm512_unpackhi_epi32(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector { _mm512_unpackhi_epi64(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::test_vm(const vector& second) const {
    return std::bit_cast<mask_type>(std::bit_cast<generic::vector<T, N>>(*this).test_vm(std::bit_cast<generic::vector<T, N>>(second)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::test_bm(const vector& second) const {
    if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm_test_epi8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm_test_epi16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm_test_epi32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm_test_epi64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm256_test_epi8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm256_test_epi16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm256_test_epi32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm256_test_epi64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm512_test_epi8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm512_test_epi16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm512_test_epi32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm512_test_epi64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::test(const vector& second) const {
    return test_bm(second);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::eq_vm(const vector& second) const {
    return std::bit_cast<mask_type>(std::bit_cast<generic::vector<T, N>>(*this).eq_vm(std::bit_cast<generic::vector<T, N>>(second)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::eq_bm(const vector& second) const {
    if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm_cmpeq_epu8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm_cmpeq_epu16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm_cmpeq_epu32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm_cmpeq_epu64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm256_cmpeq_epu8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm256_cmpeq_epu16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm256_cmpeq_epu32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm256_cmpeq_epu64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm512_cmpeq_epu8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm512_cmpeq_epu16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm512_cmpeq_epu32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm512_cmpeq_epu64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::eq(const vector& second) const {
    return eq_bm(second);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::neq_vm(const vector& second) const {
    return std::bit_cast<mask_type>(std::bit_cast<generic::vector<T, N>>(*this).neq_vm(std::bit_cast<generic::vector<T, N>>(second)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::neq_bm(const vector& second) const {
    if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm_cmpneq_epu8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm_cmpneq_epu16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm_cmpneq_epu32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm_cmpneq_epu64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm256_cmpneq_epu8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm256_cmpneq_epu16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm256_cmpneq_epu32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm256_cmpneq_epu64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm512_cmpneq_epu8_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm512_cmpneq_epu16_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm512_cmpneq_epu32_mask(raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm512_cmpneq_epu64_mask(raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::neq(const vector& second) const {
    return neq_bm(second);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::gt_vm(const vector& second) const {
    return std::bit_cast<mask_type>(std::bit_cast<generic::vector<T, N>>(*this).gt_vm(std::bit_cast<generic::vector<T, N>>(second)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::gt_bm(const vector& second) const {
    if constexpr (std::is_signed_v<T>) {
      if constexpr (is_128_bit) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return bmask_type { _mm_cmpgt_epi8_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u16)) {
          return bmask_type { _mm_cmpgt_epi16_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u32)) {
          return bmask_type { _mm_cmpgt_epi32_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u64)) {
          return bmask_type { _mm_cmpgt_epi64_mask(raw, second.raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else if constexpr (is_128_bit) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return bmask_type { _mm256_cmpgt_epi8_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u16)) {
          return bmask_type { _mm256_cmpgt_epi16_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u32)) {
          return bmask_type { _mm256_cmpgt_epi32_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u64)) {
          return bmask_type { _mm256_cmpgt_epi64_mask(raw, second.raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return bmask_type { _mm512_cmpgt_epi8_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u16)) {
          return bmask_type { _mm512_cmpgt_epi16_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u32)) {
          return bmask_type { _mm512_cmpgt_epi32_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u64)) {
          return bmask_type { _mm512_cmpgt_epi64_mask(raw, second.raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      }
    } else {
      if constexpr (is_128_bit) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return bmask_type { _mm_cmpgt_epu8_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u16)) {
          return bmask_type { _mm_cmpgt_epu16_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u32)) {
          return bmask_type { _mm_cmpgt_epu32_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u64)) {
          return bmask_type { _mm_cmpgt_epu64_mask(raw, second.raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else if constexpr (is_128_bit) {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return bmask_type { _mm256_cmpgt_epu8_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u16)) {
          return bmask_type { _mm256_cmpgt_epu16_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u32)) {
          return bmask_type { _mm256_cmpgt_epu32_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u64)) {
          return bmask_type { _mm256_cmpgt_epu64_mask(raw, second.raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      } else {
        if constexpr (sizeof(T) == sizeof(u8)) {
          return bmask_type { _mm512_cmpgt_epu8_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u16)) {
          return bmask_type { _mm512_cmpgt_epu16_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u32)) {
          return bmask_type { _mm512_cmpgt_epu32_mask(raw, second.raw) };
        } else if constexpr (sizeof(T) == sizeof(u64)) {
          return bmask_type { _mm512_cmpgt_epu64_mask(raw, second.raw) };
        } else {
          static_assert(detail::always_false<T>);
        }
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::gt(const vector& other) const {
    return gt_bm(other);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::nonzeros_vm() const {
    return std::bit_cast<vmask_type>(std::bit_cast<generic::vector<T, N>>(*this).nonzeros_vm());
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::nonzeros_bm() const {
    return neq_bm(zero());
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::nonzeros() const {
    return nonzeros_bm();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr usize vector<T, N, Env>::nonzeros_count() const {
    return nonzeros().popcount();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::zeros_vm() const {
    return std::bit_cast<vmask_type>(std::bit_cast<generic::vector<T, N>>(*this).zeros_vm());
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::zeros_bm() const {
    return eq_bm(zero());
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::zeros() const {
    return zeros_bm();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr usize vector<T, N, Env>::zeros_count() const {
    return zeros().popcount();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::vmask_type vector<T, N, Env>::msb_vm() const {
    return std::bit_cast<vmask_type>(std::bit_cast<generic::vector<T, N>>(*this).msb_vm());
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::bmask_type vector<T, N, Env>::msb_bm() const {
    if constexpr (is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm_movepi8_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm_movepi16_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm_movepi32_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm_movepi64_mask(raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm256_movepi8_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm256_movepi16_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm256_movepi32_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm256_movepi64_mask(raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return bmask_type { _mm512_movepi8_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return bmask_type { _mm512_movepi16_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return bmask_type { _mm512_movepi32_mask(raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return bmask_type { _mm512_movepi64_mask(raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>::mask_type vector<T, N, Env>::msb() const {
    return msb_bm();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr std::array<T, N> vector<T, N, Env>::to_array() const {
    return std::bit_cast<std::array<T, N>>(raw);
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr bool operator==(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (vector<T, N, Env>::is_128_bit) {
      __m128i x = _mm_xor_si128(first.raw, second.raw);
      return _mm_testz_si128(x, x);
    } else if constexpr (vector<T, N, Env>::is_256_bit) {
      __m256i x = _mm256_xor_si256(first.raw, second.raw);
      return _mm256_testz_si256(x, x);
    } else {
      __mmask16 m = _mm512_cmpeq_epi32_mask(first.raw, second.raw);
      return _mm512_kortestc(m, m);
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator~(const vector<T, N, Env>& first) {
    return first ^ vector<T, N, Env>::splat(static_cast<T>(~static_cast<T>(0)));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator&(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (vector<T, N, Env>::is_128_bit) {
      return vector<T, N, Env> { _mm_and_si128(first.raw, second.raw) };
    } else if constexpr (vector<T, N, Env>::is_256_bit) {
      return vector<T, N, Env> { _mm256_and_si256(first.raw, second.raw) };
    } else {
      return vector<T, N, Env> { _mm512_and_si512(first.raw, second.raw) };
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator&=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first & second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator|(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (vector<T, N, Env>::is_128_bit) {
      return vector<T, N, Env> { _mm_or_si128(first.raw, second.raw) };
    } else if constexpr (vector<T, N, Env>::is_256_bit) {
      return vector<T, N, Env> { _mm256_or_si256(first.raw, second.raw) };
    } else {
      return vector<T, N, Env> { _mm512_or_si512(first.raw, second.raw) };
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator|=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first | second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator^(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (vector<T, N, Env>::is_128_bit) {
      return vector<T, N, Env> { _mm_xor_si128(first.raw, second.raw) };
    } else if constexpr (vector<T, N, Env>::is_256_bit) {
      return vector<T, N, Env> { _mm256_xor_si256(first.raw, second.raw) };
    } else {
      return vector<T, N, Env> { _mm512_xor_si512(first.raw, second.raw) };
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator^=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first ^ second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator+(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (vector<T, N, Env>::is_128_bit) {
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
    } else if constexpr (vector<T, N, Env>::is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector<T, N, Env> { _mm256_add_epi8(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N, Env> { _mm256_add_epi16(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N, Env> { _mm256_add_epi32(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N, Env> { _mm256_add_epi64(first.raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector<T, N, Env> { _mm512_add_epi8(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N, Env> { _mm512_add_epi16(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N, Env> { _mm512_add_epi32(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N, Env> { _mm512_add_epi64(first.raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator+=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first + second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator-(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (vector<T, N, Env>::is_128_bit) {
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
    } else if constexpr (vector<T, N, Env>::is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector<T, N, Env> { _mm256_sub_epi8(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N, Env> { _mm256_sub_epi16(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N, Env> { _mm256_sub_epi32(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N, Env> { _mm256_sub_epi64(first.raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector<T, N, Env> { _mm512_sub_epi8(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N, Env> { _mm512_sub_epi16(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N, Env> { _mm512_sub_epi32(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N, Env> { _mm512_sub_epi64(first.raw, second.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator-=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first - second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator*(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    if constexpr (vector<T, N, Env>::is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N, Env> { _mm_mullo_epi16(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N, Env> { _mm_mullo_epi32(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N, Env> { _mm_mullo_epi64(first.raw, second.raw) };
      } else {
        return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(first) * std::bit_cast<generic::vector<T, N>>(second));
      }
    } else if constexpr (vector<T, N, Env>::is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N, Env> { _mm256_mullo_epi16(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N, Env> { _mm256_mullo_epi32(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N, Env> { _mm256_mullo_epi64(first.raw, second.raw) };
      } else {
        return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(first) * std::bit_cast<generic::vector<T, N>>(second));
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u16)) {
        return vector<T, N, Env> { _mm512_mullo_epi16(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector<T, N, Env> { _mm512_mullo_epi32(first.raw, second.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector<T, N, Env> { _mm512_mullo_epi64(first.raw, second.raw) };
      } else {
        return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(first) * std::bit_cast<generic::vector<T, N>>(second));
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator*=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return first = first * second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator<<(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(first) << std::bit_cast<generic::vector<T, N>>(second));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator<<=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    // TODO: AVX512 variable shifts
    return first = first << second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env> operator>>(const vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    return std::bit_cast<vector<T, N, Env>>(std::bit_cast<generic::vector<T, N>>(first) >> std::bit_cast<generic::vector<T, N>>(second));
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr vector<T, N, Env>& operator>>=(vector<T, N, Env>& first, const vector<T, N, Env>& second) {
    // TODO: AVX512 variable shifts
    return first = first >> second;
  }

}  // namespace lps::avx512
