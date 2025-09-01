#include "rose/game.h"

#include <algorithm>
#include <print>

#include "rose/movegen.h"

namespace rose {

  auto Game::isDraw() const -> bool {
    if (isRuleDraw())
      return true;

    // Detect stalemate
    MoveList moves;
    PrecompMoveGenInfo movegen_precomp{position()};
    MoveGen movegen{position(), movegen_precomp};
    movegen.generateMoves(moves);
    return moves.size() == 0;
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

  auto Game::printHashStack() const -> void {
    for (const u64 h : m_hash_stack)
      std::print("{:016x}\n", h);
  }

} // namespace rose
