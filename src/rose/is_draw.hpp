#pragma once

#include "rose/common.hpp"
#include "rose/hash.hpp"
#include "rose/position.hpp"
#include "rose/score.hpp"
#include "rose/util/assert.hpp"

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
      return hash_stack[height - i].full();
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

  inline auto is_insufficient_material(const Position& pos, const Hashes& hash) -> bool {
    if (hash.pawn() != 0)
      return false;

    const i32 total_piece_count = pos.total_piece_count();
    const PieceList<PieceType> white_pieces = pos.piece_list_type(Color::white);
    const PieceList<PieceType> black_pieces = pos.piece_list_type(Color::black);

    switch (total_piece_count) {
    case 2:
      // KvK
      rose_assert(white_pieces.has_only<PieceType::k>());
      rose_assert(black_pieces.has_only<PieceType::k>());
      return true;

    case 3:
      // KNvK / KBvK
      return white_pieces.has_only<PieceType::k, PieceType::n, PieceType::b>() && black_pieces.has_only<PieceType::k, PieceType::n, PieceType::b>();

    case 4:
      // KBvKB with same color bishops
      if (pos.piece_count(Color::white) == 2 && pos.piece_count(Color::black) == 2) {
        const bool only_bishops = white_pieces.has_only<PieceType::k, PieceType::b>() && black_pieces.has_only<PieceType::k, PieceType::b>();
        const bool same_color_bishops = (pos.bitboard_for<PieceType::b>() & Bitboard {0xAA55AA55AA55AA55}).popcount() != 1;
        return only_bishops && same_color_bishops;
      }
      return false;
    }

    return false;
  }

}  // namespace rose::draw
