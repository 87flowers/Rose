#include "rose/game.h"

#include <algorithm>
#include <print>

namespace rose {

  auto Game::printGameRecord() const -> void {
    std::print("> position ");
    if (m_position_stack.front() == Position::startpos()) {
      std::print("startpos");
    } else {
      std::print("fen {}", m_position_stack.front());
    }
    if (m_move_stack.size() > 0) {
      std::print(" moves");
      for (const Move m : m_move_stack) {
        std::print(" {}", m);
      }
    }
    std::print("\n");
  }

  auto Game::printHashStack() const -> void {
    for (const u64 h : m_hash_stack)
      std::print("{:016x}\n", h);
  }

} // namespace rose
