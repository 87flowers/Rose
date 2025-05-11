#pragma once

namespace rose {
  struct Engine;
  struct Game;
} // namespace rose

namespace rose::bench {

  auto run(Engine &engine, Game &game) -> void;

} // namespace rose::bench
