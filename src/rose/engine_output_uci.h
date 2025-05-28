#pragma once

#include <cstdio>
#include <print>

#include "rose/engine_output.h"
#include "rose/line.h"
#include "rose/move.h"
#include "rose/util/types.h"

namespace rose {

  struct EngineOutputUci : public EngineOutput {
    virtual ~EngineOutputUci() = default;

    auto info(EngineOutput::Info args) -> void override {
      const auto time_ms = time::cast<time::Milliseconds>(args.time);
      const u64 nps = time::nps<u64>(args.nodes, args.time);
      std::print("info depth {} score cp {} time {} nodes {} nps {} pv {}\n", args.depth, args.score, time_ms.count(), args.nodes, nps, args.pv);
    }

    auto bestmove(Move m) -> void override {
      std::print("bestmove {}\n", m);
      std::fflush(stdout);
    }
  };

} // namespace rose
