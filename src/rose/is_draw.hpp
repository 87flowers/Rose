#pragma once

#include "rose/common.hpp"
#include "rose/hash.hpp"
#include "rose/position.hpp"
#include "rose/score.hpp"

#include <optional>
#include <vector>

namespace rose::draw {

  inline auto is_stalemate(const Position& pos) -> bool {
    return !pos.is_in_check() && pos.has_no_legal_moves_slow();
  }

  inline auto is_fifty_move_draw(const Position& pos, i32 ply) -> std::optional<Score> {
    if (pos.fifty_move_clock() < 100)
      return std::nullopt;
    if (pos.is_in_check() && pos.has_no_legal_moves_slow()) [[unlikely]]
      return score::mated(ply);
    return 0;
  }

  inline auto is_repetition(const std::vector<Hash>& hash_stack, usize hash_waterline) -> bool {
    const int height = static_cast<int>(hash_stack.size()) - 1;
    const Hash current_hash = hash_stack[height];

    usize clones = 0;
    for (int i = height - 4; i >= 0; i -= 2) {
      if (hash_stack[i] == current_hash) {
        const usize clone_limit = i < hash_waterline ? 2 : 1;
        clones++;
        if (clones >= clone_limit)
          return true;
      }
    }
    return false;
  }

}  // namespace rose::draw
