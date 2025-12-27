#pragma once

#include "rose/common.hpp"

#if __BMI2__
#include <immintrin.h>
#endif

namespace rose {

  constexpr auto pext_slow(u64 x, u64 m) -> u64 {
    u64 result = 0;
    for (u64 b = 1; m != 0; b += b) {
      if ((x & m & -m) != 0)
        result |= b;
      m &= m - 1;
    }
    return result;
  }

  constexpr auto pext(u64 x, u64 m) -> u64 {
#if __BMI2__
    if (std::is_constant_evaluated())
      return pext_slow(x, m);
    return _pext_u64(x, m);
#else
    return pext_slow(x, m);
#endif
  }

  constexpr auto pdep_slow(u64 x, u64 m) {
    u64 result = 0;
    for (u64 b = 1; m != 0; b += b) {
      if ((x & b) != 0)
        result |= m & -m;
      m &= m - 1;
    }
    return result;
  }

  constexpr auto pdep(u64 x, u64 m) -> u64 {
#if __BMI2__
    if (std::is_constant_evaluated())
      return pdep_slow(x, m);
    return _pdep_u64(x, m);
#else
    return pdep_slow(x, m);
#endif
  }

}  // namespace rose
