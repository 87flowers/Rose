#pragma

#include "rose/util/types.h"

namespace rose::tunable {

  inline constexpr i32 history_bonus_scale = 100;
  inline constexpr i32 history_bonus_const = -30;
  inline constexpr i32 history_bonus_max = 10000;
  inline constexpr i32 history_max = 1 << 14;

  inline constexpr usize lmr_move_threshold = 5;
  inline constexpr i32 lmr_depth_threshold = 3;
  inline constexpr i32 lmr_base_const = 3;
  inline constexpr i32 lmr_base_scale = 4;

  inline constexpr i32 rfp_max_depth = 6;
  inline constexpr i32 rfp_margin = 100;

  inline constexpr i32 iid_min_depth = 6;
  inline constexpr i32 iid_shallow_depth = 4;

} // namespace rose::tunable
