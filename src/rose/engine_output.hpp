#pragma once

#include "rose/common.hpp"
#include "rose/line.hpp"
#include "rose/move.hpp"
#include "rose/util/time.hpp"

namespace rose {

  struct EngineOutput {
    virtual ~EngineOutput() = default;

    struct Info {
      i32 depth;
      i32 score;
      time::Duration time;
      u64 nodes;
      const Line& pv;
    };

    virtual auto info(Info args) -> void = 0;

    virtual auto bestmove(Move m) -> void = 0;
  };

}  // namespace rose
