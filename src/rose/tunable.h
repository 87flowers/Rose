#pragma

#include "rose/util/types.h"

namespace rose::tunable {

  inline constexpr i32 history_bonus_scale = 100;
  inline constexpr i32 history_bonus_const = -30;
  inline constexpr i32 history_bonus_max = 10000;
  inline constexpr i32 history_max = 1 << 14;

} // namespace rose::tunable
