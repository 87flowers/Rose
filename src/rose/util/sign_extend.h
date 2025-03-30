#pragma once

#include "rose/types.h"

namespace rose {

  template <usize bitsize, typename Dest> inline constexpr auto signExtend(auto src) {
    struct {
      Dest x : bitsize;
    } tmp;
    tmp.x = src;
    return static_cast<Dest>(tmp.x);
  }

} // namespace rose
