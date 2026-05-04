#pragma once

#include "rose/common.hpp"
#include "rose/engine_output.hpp"
#include "rose/move.hpp"

namespace rose {

  struct EngineOutputNull : public EngineOutput {
    virtual ~EngineOutputNull() = default;

    auto info(EngineOutput::Info args) -> void override {
      rose_unused(args);
    }

    auto bestmove(Move m) -> void override {
      rose_unused(m);
    }
  };

}  // namespace rose
