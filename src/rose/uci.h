#pragma once

#include <string_view>

namespace rose {

  struct Engine;
  struct Game;

  auto uciParseCommand(Engine &engine, Game &game, std::string_view cmd) -> void;

} // namespace rose
