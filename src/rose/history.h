#pragma once

#include <array>

#include "rose/move.h"
#include "rose/util/types.h"

namespace rose {

  struct History {
  private:
    std::array<std::array<i16, 64>, 64> m_quiet_sd{};

  public:
    auto clear() -> void;

    auto updateQuietHistory(i32 sign, Move m, i32 depth) -> void;
    auto getHistory(Move m) const -> i32;
  };

} // namespace rose
