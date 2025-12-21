#pragma once

#include "lps/avx512/basic_bit_mask.def.hpp"
#include "lps/avx512/basic_vector_mask.def.hpp"
#include "lps/avx512/vector.def.hpp"
#include "lps/detail/always_false.hpp"
#include "lps/stdint.hpp"

#include <array>
#include <bit>

namespace lps::avx512 {

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env>::basic_bit_mask(raw_type value) :
      raw(value) {
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env>::basic_bit_mask(bits_type value)
    requires(!std::is_same_v<raw_type, bits_type>)
      :
      raw(std::bit_cast<raw_type>(value)) {
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env> basic_bit_mask<T, N, Env>::zero() {
    return basic_bit_mask { bits_type { 0 } };
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env> basic_bit_mask<T, N, Env>::splat(bool value) {
    return value ? ~zero() : zero();
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr void basic_bit_mask<T, N, Env>::set(usize index, bool value) {
    auto bit1 = static_cast<raw_type>(static_cast<bits_type>(1) << index);
    auto bitv = static_cast<raw_type>(static_cast<bits_type>(value) << index);
    raw = (andnot(basic_bit_mask { bit1 }) | basic_bit_mask { bitv }).raw;
  }

  template<class T, usize N, class Env>
  template<class U>
  LPS_INLINE constexpr typename Env::template bit_mask<U, detail::clamped_size<U, N>> basic_bit_mask<T, N, Env>::convert() const {
    return typename Env::template bit_mask<U, detail::clamped_size<U, N>> { raw };
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_bit_mask<T, N, Env>::mask(const V& v1) const {
    if constexpr (V::is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm_maskz_mov_epi8(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm_maskz_mov_epi16(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm_maskz_mov_epi32(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm_maskz_mov_epi64(raw, v1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (V::is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm256_maskz_mov_epi8(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm256_maskz_mov_epi16(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm256_maskz_mov_epi32(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm256_maskz_mov_epi64(raw, v1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm512_maskz_mov_epi8(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm512_maskz_mov_epi16(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm512_maskz_mov_epi32(raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm512_maskz_mov_epi64(raw, v1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_bit_mask<T, N, Env>::select(const V& v0, const V& v1) const {
    if constexpr (V::is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm_mask_blend_epi8(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm_mask_blend_epi16(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm_mask_blend_epi32(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm_mask_blend_epi64(raw, v0.raw, v1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (V::is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm256_mask_blend_epi8(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm256_mask_blend_epi16(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm256_mask_blend_epi32(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm256_mask_blend_epi64(raw, v0.raw, v1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm512_mask_blend_epi8(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm512_mask_blend_epi16(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm512_mask_blend_epi32(raw, v0.raw, v1.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm512_mask_blend_epi64(raw, v0.raw, v1.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  template<class V>
    requires std::is_same_v<V, typename Env::template vector<typename V::element_type, N>>
  LPS_INLINE constexpr V basic_bit_mask<T, N, Env>::compress(const V& v) const {
    if constexpr (V::is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm_maskz_compress_epi8(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm_maskz_compress_epi16(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm_maskz_compress_epi32(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm_maskz_compress_epi64(raw, v.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (V::is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm256_maskz_compress_epi8(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm256_maskz_compress_epi16(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm256_maskz_compress_epi32(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm256_maskz_compress_epi64(raw, v.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return V { _mm512_maskz_compress_epi8(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return V { _mm512_maskz_compress_epi16(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return V { _mm512_maskz_compress_epi32(raw, v.raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return V { _mm512_maskz_compress_epi64(raw, v.raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env> basic_bit_mask<T, N, Env>::andnot(const basic_bit_mask<T, N, Env>& second) const {
    if constexpr (sizeof(raw_type) == sizeof(u8)) {
      return basic_bit_mask { _kandn_mask8(second.raw, raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u16)) {
      return basic_bit_mask { _kandn_mask16(second.raw, raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u32)) {
      return basic_bit_mask { _kandn_mask32(second.raw, raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u64)) {
      return basic_bit_mask { _kandn_mask64(second.raw, raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr usize basic_bit_mask<T, N, Env>::popcount() const {
    return static_cast<usize>(std::popcount(to_bits()));
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr std::array<T, N> basic_bit_mask<T, N, Env>::to_array() const {
    return raw.to_vector().to_array();
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr detail::bit_mask_base_t<N> basic_bit_mask<T, N, Env>::to_bits() const {
    return std::bit_cast<bits_type>(raw);
  }

  template<class T, usize N, class Env>
  [[nodiscard]] LPS_INLINE constexpr typename Env::template vector<T, N> basic_bit_mask<T, N, Env>::to_vector() const {
    using vector_type = typename Env::template vector<T, N>;
    if constexpr (vector_type::is_128_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector_type { _mm_movm_epi8(raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector_type { _mm_movm_epi16(raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector_type { _mm_movm_epi32(raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector_type { _mm_movm_epi64(raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else if constexpr (vector_type::is_256_bit) {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector_type { _mm256_movm_epi8(raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector_type { _mm256_movm_epi16(raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector_type { _mm256_movm_epi32(raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector_type { _mm256_movm_epi64(raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    } else {
      if constexpr (sizeof(T) == sizeof(u8)) {
        return vector_type { _mm512_movm_epi8(raw) };
      } else if constexpr (sizeof(T) == sizeof(u16)) {
        return vector_type { _mm512_movm_epi16(raw) };
      } else if constexpr (sizeof(T) == sizeof(u32)) {
        return vector_type { _mm512_movm_epi32(raw) };
      } else if constexpr (sizeof(T) == sizeof(u64)) {
        return vector_type { _mm512_movm_epi64(raw) };
      } else {
        static_assert(detail::always_false<T>);
      }
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr bool operator==(const basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second) {
    return first.raw == second.raw;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env> operator~(const basic_bit_mask<T, N, Env>& first) {
    using raw_type = typename basic_bit_mask<T, N, Env>::raw_type;
    if constexpr (sizeof(raw_type) == sizeof(u8)) {
      return basic_bit_mask<T, N, Env> { _knot_mask8(first.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u16)) {
      return basic_bit_mask<T, N, Env> { _knot_mask16(first.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u32)) {
      return basic_bit_mask<T, N, Env> { _knot_mask32(first.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u64)) {
      return basic_bit_mask<T, N, Env> { _knot_mask64(first.raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env> operator&(const basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second) {
    using raw_type = typename basic_bit_mask<T, N, Env>::raw_type;
    if constexpr (sizeof(raw_type) == sizeof(u8)) {
      return basic_bit_mask<T, N, Env> { _kand_mask8(first.raw, second.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u16)) {
      return basic_bit_mask<T, N, Env> { _kand_mask16(first.raw, second.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u32)) {
      return basic_bit_mask<T, N, Env> { _kand_mask32(first.raw, second.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u64)) {
      return basic_bit_mask<T, N, Env> { _kand_mask64(first.raw, second.raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env>& operator&=(basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second) {
    return first = first & second;
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env> operator|(const basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second) {
    using raw_type = typename basic_bit_mask<T, N, Env>::raw_type;
    if constexpr (sizeof(raw_type) == sizeof(u8)) {
      return basic_bit_mask<T, N, Env> { _kor_mask8(first.raw, second.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u16)) {
      return basic_bit_mask<T, N, Env> { _kor_mask16(first.raw, second.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u32)) {
      return basic_bit_mask<T, N, Env> { _kor_mask32(first.raw, second.raw) };
    } else if constexpr (sizeof(raw_type) == sizeof(u64)) {
      return basic_bit_mask<T, N, Env> { _kor_mask64(first.raw, second.raw) };
    } else {
      static_assert(detail::always_false<T>);
    }
  }

  template<class T, usize N, class Env>
  LPS_INLINE constexpr basic_bit_mask<T, N, Env>& operator|=(basic_bit_mask<T, N, Env>& first, const basic_bit_mask<T, N, Env>& second) {
    return first = first | second;
  }

}  // namespace lps::avx512
