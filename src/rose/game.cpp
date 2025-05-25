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
      for (int i = 0; i < m_move_stack.size(); i++) {
        std::print(" {}", PrintWithPosition{m_position_stack[i], m_move_stack[i]});
      }
    }
    std::print("\n");
  }

} // namespace rose
