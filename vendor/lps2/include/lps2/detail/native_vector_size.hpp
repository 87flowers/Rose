#pragma once

#include "lps2/arch.hpp"
#include "lps2/stdint.hpp"

namespace lps2::detail {

  inline static constexpr usize native_vector_byte_size =
#if LPS2_ARCH_AVX512BASE
    64
#elif LPS2_ARCH_AVX2
    32
#elif LPS2_ARCH_X86_64
    16
#elif LPS2_ARCH_AA64
    16
#else
    16
#endif
    ;

  template<class T>
  inline static constexpr usize native_vector_size = native_vector_byte_size * sizeof(u8) / sizeof(T);

}  // namespace lps2::detail
