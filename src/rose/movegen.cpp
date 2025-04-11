#include "rose/movegen.h"

#include <bit>
#include <tuple>

#include "rose/common.h"
#include "rose/position.h"
#include "rose/util/pext.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose::movegen {

  template <typename T> auto MoveList::write(typename T::Mask16 mask, T v) -> void {
    const usize count = std::popcount(mask);
    rose_assert(len + count < capacity());
    vec::compressstore16(data.data() + len, mask, v);
    len += count;
  }

  static auto generateMovesNoCheckers(MoveList &moves, const Position &position, Square king_sq) -> void;
  static auto generateMovesOneCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void;
  static auto generateMovesTwoCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void;

  static auto writeKingMovesWithCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void;

  auto generateMoves(MoveList &moves, const Position &position) -> void {
    const Color active_color = position.activeColor();
    const Square king_sq = position.kingSq(active_color);
    const u16 checkers = position.attackTable(active_color.invert()).r[king_sq.raw];
    const int checkers_count = std::popcount(checkers);

    switch (checkers_count) {
    case 0:
      return generateMovesNoCheckers(moves, position, king_sq);
    case 1:
      return generateMovesOneCheckers(moves, position, king_sq, checkers);
    default:
      return generateMovesTwoCheckers(moves, position, king_sq, checkers);
    }
  }

  static auto generateMovesNoCheckers(MoveList &moves, const Position &position, Square king_sq) -> void {}

  static auto generateMovesOneCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void {}

  static auto generateMovesTwoCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void {
    writeKingMovesWithCheckers(moves, position, king_sq, checkers);
  }

  static auto writeKingMovesWithCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void {
    const Color active_color = position.activeColor();
    const Wordboard &attack_table = position.attackTable(position.activeColor().invert());

    const auto [king_leaps, leaps_valid] = geometry::adjacents(king_sq);
    const auto king_leaps16 = vec::zext8to16_lo(king_leaps);

    const v128 places = vec::permute8(v512::from128(king_leaps), position.board().z).to128();
    const u16 occupied = places.nonzero8() & leaps_valid;
    const u16 color = places.msb8();
    const u16 friendly = (color ^ ~active_color.toBitboard()) & occupied;

    const u16 no_attackers = vec::permute16_mz(leaps_valid, v512::from128(king_leaps16), attack_table.z[0], attack_table.z[1]).to128().zero16();

    u16 destinations = leaps_valid & ~friendly & no_attackers;

    // TODO: Vectorize
    for (; checkers != 0; checkers &= checkers - 1) {
      const int checker_index = std::countr_zero(checkers);
      const PieceType checker_ptype = position.pieceListType(active_color.invert()).m[checker_index];
      const Square checker_sq = position.pieceListSq(active_color.invert()).m[checker_index];

      const auto [checker_file, checker_rank] = checker_sq.toFileAndRank();
      const auto [king_file, king_rank] = king_sq.toFileAndRank();

      const bool same_file = checker_file == king_file;
      const bool same_rank = checker_rank == king_rank;

      if (same_file || same_rank) {
        if (checker_ptype.raw & PieceType::r) {
          destinations &= same_file ? ~geometry::adjacents_same_file_mask : ~geometry::adjacents_same_rank_mask;
        }
      } else {
        if (checker_ptype.raw & PieceType::b) {
          const bool rel_file = checker_file > king_file;
          const bool rel_rank = checker_rank > king_rank;
          const bool is_antidiagonal = rel_file != rel_rank;
          destinations &= is_antidiagonal ? ~geometry::adjacents_antidiagonal_mask : ~geometry::adjacents_diagonal_mask;
        }
      }
    }

    const v128 write_vector = vec::shl16(king_leaps16, 6) | v128::broadcast16(king_sq.raw);
    moves.write(destinations, write_vector);
  }

} // namespace rose::movegen
