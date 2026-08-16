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

  inline auto has_upcoming_repetition(const Position& pos, const std::vector<Hashes>& hash_stack, usize hash_waterline) -> bool {
    const int height = static_cast<int>(hash_stack.size()) - 1;
    const int end = std::min<int>(std::min<int>(pos.fifty_move_clock(), pos.ply_since_null()), height);

    const auto hashes_at = [&hash_stack, height](int i) -> Hashes {
      return hash_stack[height - i];
    };
    const Hashes current_hashes = hashes_at(0);

    const Color stm = pos.stm();

    for (int i = 3; i <= end; i += 2) {
      const Hashes h = hashes_at(i);

      if (h.color[!stm.to_index()] != current_hashes.color[!stm.to_index()])
        continue;

      const Bitboard bb_diff = h.occupancy ^ current_hashes.occupancy;
      if (bb_diff.popcount() != 2)
        continue;

      const Hash full_hash_diff = h.full ^ current_hashes.full;

      const Square from = (bb_diff & current_hashes.occupancy).lsb();
      const Square to = (bb_diff & h.occupancy).lsb();
      const Place src = pos.board()[from];

      if (full_hash_diff == (hash::move ^ hash::move_piece(from, to, src)) && pos.attack_table(stm).read(to).is_set(src.id())) {
        const usize clone_limit = (height - i) < hash_waterline ? 2 : 1;
        if (clone_limit == 1)
          return true;
        for (int j = i + 2; j <= end; j++)
          if (h.full == hashes_at(j).full)
            return true;
      }
    }
    return false;
  }

}  // namespace rose::draw
