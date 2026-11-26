#pragma once

#include "lps2/arch.hpp"
#include "lps2/bit_mask.hpp"
#include "lps2/bit_mask_ops.hpp"
#include "lps2/detail/native_vector_size.hpp"
#include "lps2/vector.hpp"
#include "lps2/vector_mask.hpp"
#include "lps2/vector_mask_ops.hpp"
#include "lps2/vector_ops.hpp"

namespace lps2::prelude {

#if LPS2_PREFERS_BIT_MASK
  template<class T, usize N>
  using mask = bit_mask<T, N>;
#endif
#if LPS2_PREFERS_VECTOR_MASK
  template<class T, usize N>
  using mask = vector_mask<T, N>;
#endif

  using u8xN = vector<u8, detail::native_vector_size<u8>>;
  using i8xN = vector<i8, detail::native_vector_size<u8>>;
  using bm8xN = bit_mask<i8, detail::native_vector_size<u8>>;
  using vm8xN = vector_mask<i8, detail::native_vector_size<u8>>;
  using m8xN = mask<i8, detail::native_vector_size<u8>>;

  using u16xN = vector<u16, detail::native_vector_size<u16>>;
  using i16xN = vector<i16, detail::native_vector_size<u16>>;
  using bm16xN = bit_mask<i16, detail::native_vector_size<u16>>;
  using vm16xN = vector_mask<i16, detail::native_vector_size<u16>>;
  using m16xN = mask<i16, detail::native_vector_size<u16>>;

  using u32xN = vector<u32, detail::native_vector_size<u32>>;
  using i32xN = vector<i32, detail::native_vector_size<u32>>;
  using bm32xN = bit_mask<i32, detail::native_vector_size<u32>>;
  using vm32xN = vector_mask<i32, detail::native_vector_size<u32>>;
  using m32xN = mask<i32, detail::native_vector_size<u32>>;

  using u64xN = vector<u64, detail::native_vector_size<u64>>;
  using i64xN = vector<i64, detail::native_vector_size<u64>>;
  using bm64xN = bit_mask<i64, detail::native_vector_size<u64>>;
  using vm64xN = vector_mask<i64, detail::native_vector_size<u64>>;
  using m64xN = mask<i64, detail::native_vector_size<u64>>;

  using u8x16 = vector<u8, 16>;
  using i8x16 = vector<i8, 16>;
  using bm8x16 = bit_mask<i8, 16>;
  using vm8x16 = vector_mask<i8, 16>;
  using m8x16 = mask<i8, 16>;

  using u8x32 = vector<u8, 32>;
  using i8x32 = vector<i8, 32>;
  using bm8x32 = bit_mask<i8, 32>;
  using vm8x32 = vector_mask<i8, 32>;
  using m8x32 = mask<i8, 32>;

  using u8x64 = vector<u8, 64>;
  using i8x64 = vector<i8, 64>;
  using bm8x64 = bit_mask<i8, 64>;
  using vm8x64 = vector_mask<i8, 64>;
  using m8x64 = mask<i8, 64>;

  using u8x128 = vector<u8, 128>;
  using i8x128 = vector<i8, 128>;
  using bm8x128 = bit_mask<i8, 128>;
  using vm8x128 = vector_mask<i8, 128>;
  using m8x128 = mask<i8, 128>;

  using u16x8 = vector<u16, 8>;
  using i16x8 = vector<i16, 8>;
  using bm16x8 = bit_mask<i16, 8>;
  using vm16x8 = vector_mask<i16, 8>;
  using m16x8 = mask<i16, 8>;

  using u16x16 = vector<u16, 16>;
  using i16x16 = vector<i16, 16>;
  using bm16x16 = bit_mask<i16, 16>;
  using vm16x16 = vector_mask<i16, 16>;
  using m16x16 = mask<i16, 16>;

  using u16x32 = vector<u16, 32>;
  using i16x32 = vector<i16, 32>;
  using bm16x32 = bit_mask<i16, 32>;
  using vm16x32 = vector_mask<i16, 32>;
  using m16x32 = mask<i16, 32>;

  using u16x64 = vector<u16, 64>;
  using i16x64 = vector<i16, 64>;
  using bm16x64 = bit_mask<i16, 64>;
  using vm16x64 = vector_mask<i16, 64>;
  using m16x64 = mask<i16, 64>;

  using u32x4 = vector<u32, 4>;
  using i32x4 = vector<i32, 4>;
  using bm32x4 = bit_mask<i32, 4>;
  using vm32x4 = vector_mask<i32, 4>;
  using m32x4 = mask<i32, 4>;

  using u64x8 = vector<u64, 8>;
  using i64x8 = vector<i64, 8>;
  using bm64x8 = bit_mask<i64, 8>;
  using vm64x8 = vector_mask<i64, 8>;
  using m64x8 = mask<i64, 8>;

}  // namespace lps2::prelude
