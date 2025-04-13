#pragma once

#include <string_view>

namespace rose {

  struct Game;

  auto uciParseCommand(Game &game, std::string_view cmd) -> void;

} // namespace rose
