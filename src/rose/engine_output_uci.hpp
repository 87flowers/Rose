#pragma once

#include "rose/common.hpp"
#include "rose/engine_output.hpp"
#include "rose/move.hpp"

#include <cstdio>
#include <fmt/format.h>

namespace rose {

  struct EngineOutputUci : public EngineOutput {
  private:
    MoveFormat format;

  public:
    EngineOutputUci(MoveFormat format) :
        format(format) {
    }

    virtual ~EngineOutputUci() = default;

    virtual auto info(EngineOutput::Info args) -> void override {
      const auto time_ms = time::cast<time::Milliseconds>(args.time);
      const u64 nps = time::nps<u64>(args.nodes, args.time);
      fmt::print("info depth {} score cp {} time {} nodes {} nps {} pv {}\n",
                 args.depth,
                 args.score,
                 time_ms.count(),
                 args.nodes,
                 nps,
                 args.pv.to_string(format));
    }

    virtual auto bestmove(Move m) -> void override {
      fmt::print("bestmove {}\n", m.to_string(format));
      std::fflush(stdout);
    }
  };

}  // namespace rose
