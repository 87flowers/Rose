#pragma once

#include "lps2/arch.hpp"
#include "lps2/detail/bit_mask_base.hpp"
#include "lps2/stdint.hpp"

#if LPS2_ARCH_X86_64
#include <immintrin.h>
#endif

#if LPS2_ARCH_AARCH64
#include <arm_neon.h>
#endif

namespace lps2::detail {

  template<class T, usize N>
  struct bit_mask_arch_type;

  template<class T, usize N>
  using bit_mask_arch_type_t = typename bit_mask_arch_type<T, N>::type;

#if LPS2_ARCH_AVX512BASE

  template<class T>
  struct bit_mask_arch_type<T, 2> {
    using type = __mmask8;
  };

  template<class T>
  struct bit_mask_arch_type<T, 4> {
    using type = __mmask8;
  };

  template<class T>
  struct bit_mask_arch_type<T, 8> {
    using type = __mmask8;
  };

  template<class T>
  struct bit_mask_arch_type<T, 16> {
    using type = __mmask16;
  };

  template<class T>
  struct bit_mask_arch_type<T, 32> {
    using type = __mmask32;
  };

  template<class T>
  struct bit_mask_arch_type<T, 64> {
    using type = __mmask64;
  };

#else

  template<class T, usize N>
  struct bit_mask_arch_type {
    using type = bit_mask_base_t<N>;
  };

#endif

}  // namespace lps2::detail
