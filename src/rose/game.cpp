#include "rose/game.h"

#include <print>

namespace rose {

  auto Game::reset() -> void { setPositionStartpos(); }

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

} // namespace rose
