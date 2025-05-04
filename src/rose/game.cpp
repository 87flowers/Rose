#include "rose/game.h"

#include <algorithm>
#include <print>

namespace rose {

  auto Game::isRepetition() const -> bool {
    const int height = static_cast<int>(m_hash_stack.size()) - 1;

    const u64 h = hash();
    usize clones = 0;

    for (int i = height - 4; i >= 0; i -= 2) {
      if (m_hash_stack[i] == h) {
        const usize clone_limit = (i < m_hash_waterline) ? 2 : 1;
        clones++;
        if (clones >= clone_limit)
          return true;
      }
    }

    return false;
  }

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
