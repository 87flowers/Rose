#pragma once

#include <immintrin.h>

#include "rose/util/types.h"

namespace rose {

  constexpr auto pextSlow(u64 x, u64 m) -> u64 {
    u64 result = 0;
    for (u64 b = 1; m != 0; b += b) {
      if ((x & m & -m) != 0)
        result |= b;
      m &= m - 1;
    }
    return result;
  }

  constexpr auto pext(u64 x, u64 m) -> u64 {
    if (std::is_constant_evaluated())
      return pextSlow(x, m);
    return _pext_u64(x, m);
  }

  constexpr auto pdepSlow(u64 x, u64 m) {
    u64 result = 0;
    for (u64 b = 1; m != 0; b += b) {
      if ((x & b) != 0)
        result |= m & -m;
      m &= m - 1;
    }
    return result;
  }

  constexpr auto pdep(u64 x, u64 m) -> u64 {
    if (std::is_constant_evaluated())
      return pdepSlow(x, m);
    return _pdep_u64(x, m);
  }

} // namespace rose
