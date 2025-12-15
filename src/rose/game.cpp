#include "rose/game.hpp"

#include <fmt/format.h>

namespace rose {

  auto Game::print_game_record() const -> void {
    fmt::print("> position ");
    if (m_position_stack.front() == Position::startpos()) {
      fmt::print("startpos");
    } else {
      fmt::print("fen {}", m_position_stack.front());
    }
    if (m_move_stack.size() > 0) {
      fmt::print(" moves");
      for (const Move m : m_move_stack) {
        fmt::print(" {}", m);
      }
    }
    fmt::print("\n");
  }

  // auto Game::print_hash_stack() const -> void {
  //   for (const u64 h : m_hash_stack)
  //     fmt::print("{:016x}\n", h);
  // }

}  // namespace rose
