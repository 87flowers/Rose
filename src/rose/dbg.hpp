#pragma once

#include "rose/common.hpp"

namespace rose::dbg {

  auto dbg_hit(usize slot, bool value) -> void;
  auto dbg_stats(usize slot, i64 value) -> void;

  auto print() -> void;
  auto clear() -> void;

}  // namespace rose::dbg
