#pragma once

#include "rose/common.hpp"
#include "rose/engine_output.hpp"
#include "rose/move.hpp"

#include <cstdio>
#include <fmt/format.h>

namespace rose {

  template<class Interface>
  struct EngineOutputXboard : public EngineOutput {
  private:
    MoveFormat format;
    Interface& interface;

  public:
    EngineOutputXboard(Interface& interface, MoveFormat format) :
        interface(interface),
        format(format) {
    }

    virtual ~EngineOutputXboard() = default;

    auto info(EngineOutput::Info args) -> void override {
      const auto time_cs = time::cast<time::Centiseconds>(args.time);
      fmt::print("{}\t{}\t{}\t{}\t{}\n",
                 args.depth,
                 args.score,
                 time_cs.count(),  // centiseconds
                 args.nodes,
                 args.pv.to_string(format));
    }

    auto bestmove(Move m) -> void override {
      interface.xboard_engine_moved(m);
      fmt::print("move {}\n", m.to_string(format));
      std::fflush(stdout);
    }
  };

}  // namespace rose
