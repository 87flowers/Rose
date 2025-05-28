#pragma once

#include "rose/engine_output.h"

namespace rose {

  struct EngineOutputNull : public EngineOutput {
    virtual ~EngineOutputNull() = default;

    auto info(EngineOutput::Info) -> void override {}

    auto bestmove(Move) -> void override {}
  };

} // namespace rose
