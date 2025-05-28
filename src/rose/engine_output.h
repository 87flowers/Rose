#pragma once

#include "rose/line.h"
#include "rose/move.h"
#include "rose/util/types.h"

namespace rose {

  struct EngineOutput {
    virtual ~EngineOutput() = default;

    struct Info {
      i32 depth;
      i32 score;
      time::Duration time;
      u64 nodes;
      const Line &pv;
    };

    virtual auto info(Info args) -> void = 0;

    virtual auto bestmove(Move m) -> void = 0;
  };

} // namespace rose
