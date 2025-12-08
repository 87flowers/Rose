#pragma once

#include "lps/stdint.hpp"

#if defined(__AVX512F__) && __AVX512F__

#define LPS_AVX512 1

#include "lps/avx512/avx512.hpp"

namespace lps {
  using environment = avx512::environment;
}  // namespace lps

#elif defined(__AVX2__) && __AVX2__

#define LPS_AVX2 1

#include "lps/avx2/avx2.hpp"

namespace lps {
  using environment = avx2::environment;
}  // namespace lps

#elif defined(__SSE4_2__) && __SSE4_2__

#define LPS_SSE4_2 1

#include "lps/sse4_2/sse4_2.hpp"

namespace lps {
  using environment = sse4_2::environment;
}  // namespace lps

#else

#include "lps/generic/generic.hpp"

namespace lps {
  using environment = generic::environment;
}  // namespace lps

#endif

namespace lps::prelude {

  template<class T, usize N>
  using vector = environment::vector<T, N>;

  template<class T, usize N>
  using vector_mask = environment::vector_mask<T, N>;

  template<class T, usize N>
  using mask = environment::mask<T, N>;

  using u8x16 = vector<u8, 16>;
  using i8x16 = vector<i8, 16>;
  using vm8x16 = vector_mask<u8, 16>;
  using m8x16 = mask<u8, 16>;

  using u8x32 = vector<u8, 32>;
  using i8x32 = vector<i8, 32>;
  using vm8x32 = vector_mask<u8, 32>;
  using m8x32 = mask<u8, 32>;

  using u8x64 = vector<u8, 64>;
  using i8x64 = vector<i8, 64>;
  using vm8x64 = vector_mask<u8, 64>;
  using m8x64 = mask<u8, 64>;

  using u8x128 = vector<u8, 128>;
  using i8x128 = vector<i8, 128>;
  using vm8x128 = vector_mask<u8, 128>;
  using m8x128 = mask<u8, 128>;

  using u16x8 = vector<u16, 8>;
  using i16x8 = vector<i16, 8>;
  using vm16x8 = vector_mask<u16, 8>;
  using m16x8 = mask<u16, 8>;

  using u16x16 = vector<u16, 16>;
  using i16x16 = vector<i16, 16>;
  using vm16x16 = vector_mask<u16, 16>;
  using m16x16 = mask<u16, 16>;

  using u16x32 = vector<u16, 32>;
  using i16x32 = vector<i16, 32>;
  using vm16x32 = vector_mask<u16, 32>;
  using m16x32 = mask<u16, 32>;

  using u16x64 = vector<u16, 64>;
  using i16x64 = vector<i16, 64>;
  using vm16x64 = vector_mask<u16, 64>;
  using m16x64 = mask<u16, 64>;

  using u32x4 = vector<u32, 4>;
  using i32x4 = vector<i32, 4>;
  using vm32x4 = vector_mask<u32, 4>;
  using m32x4 = mask<u32, 4>;

  using u64x8 = vector<u64, 8>;
  using i64x8 = vector<i64, 8>;
  using vm64x8 = vector_mask<u64, 8>;
  using m64x8 = mask<u64, 8>;

}  // namespace lps::prelude
