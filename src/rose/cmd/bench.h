#pragma once

namespace rose {
  struct Engine;
  struct Game;
} // namespace rose

namespace rose::bench {

  auto run(bool print_output = false) -> void;

} // namespace rose::bench
