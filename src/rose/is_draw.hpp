#pragma once

#include "rose/board.hpp"
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

  inline auto is_insufficient_material(const Position& pos) -> bool {
    switch (pos.piece_count()) {
    case 2:
      return true;
    case 3:
      return pos.piece_list_type(Color::white).piece_mask_for<PieceType::p, PieceType::r, PieceType::q>().is_empty() &&
             pos.piece_list_type(Color::black).piece_mask_for<PieceType::p, PieceType::r, PieceType::q>().is_empty();
    case 4:
      if (pos.piece_count(Color::white) == 2) {
        rose_assert(pos.piece_count(Color::black) == 2);
        const PieceId white_last_piece = (pos.piece_list_type(Color::white).present() & ~PieceMask::king()).lsb();
        const PieceId black_last_piece = (pos.piece_list_type(Color::black).present() & ~PieceMask::king()).lsb();
        return pos.what_is(Color::white, white_last_piece) == PieceType::b && pos.what_is(Color::black, black_last_piece) == PieceType::b &&
               pos.where_is(Color::white, white_last_piece).parity() == pos.where_is(Color::black, black_last_piece).parity();
      }
      return false;
    default:
      return false;
    }
  }

}  // namespace rose::draw
