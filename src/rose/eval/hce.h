#pragma once

#include "rose/util/types.h"

namespace rose {
  struct Position;
}

namespace rose::eval {

  auto hce(const Position &position) -> i32;

} // namespace rose::eval
