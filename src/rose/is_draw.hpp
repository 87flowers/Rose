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

  inline auto is_repetition(const Position& pos, const std::vector<Hashes>& hash_stack, usize hash_waterline) -> bool {
    const int height = static_cast<int>(hash_stack.size()) - 1;
    const int end = std::min<int>(std::min<int>(pos.fifty_move_clock(), pos.ply_since_null()), height);

    const auto hash_at = [&hash_stack, height](int i) -> Hash {
      return hash_stack[height - i].full;
    };
    const Hash current_hash = hash_at(0);

    usize clones = 0;
    for (int i = 4; i <= end; i += 2) {
      if (hash_at(i) == current_hash) {
        const usize clone_limit = (height - i) < hash_waterline ? 2 : 1;
        clones++;
        if (clones >= clone_limit)
          return true;
      }
    }
    return false;
  }

  //  inline auto is_upcoming_repetition(const Position& pos, const std::vector<Hash>& hash_stack, usize hash_waterline) -> bool {
  //    const int height = static_cast<int>(hash_stack.size()) - 1;
  //    const int end = std::min<int>(pos.fifty_move_clock(), height);
  //
  //    const auto hash_at = [&hash_stack, height](int i) {
  //      return hash_stack[height - i];
  //    };
  //    const Hash current_hash = hash_at(0);
  //
  //    usize clones = 0;
  //    for (int i = 4; i <= end; i += 2) {
  //      const Hash diff = hash_at(i) ^ current_hash;
  //      if () {
  //        const usize clone_limit = (height - i) < hash_waterline ? 2 : 1;
  //        clones++;
  //        if (clones >= clone_limit)
  //          return true;
  //      }
  //    }
  //    return false;
  //  }

}  // namespace rose::draw
