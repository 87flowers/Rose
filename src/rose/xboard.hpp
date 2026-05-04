#pragma once

#include "rose/common.hpp"
#include "rose/util/time.hpp"

#include <array>
#include <optional>

namespace rose {

  struct XBoardState {
    bool force = true;

    std::array<time::Duration, 2> clocks;

    std::optional<i32> tc_sd;
    std::optional<time::Duration> tc_st;
    std::optional<time::Duration> tc_base;
    std::optional<time::Duration> tc_increment;
  };

}  // namespace rose
