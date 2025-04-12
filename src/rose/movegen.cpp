#include "rose/movegen.h"

#include <bit>
#include <cstring>
#include <tuple>

#include "rose/common.h"
#include "rose/geometry.h"
#include "rose/pawns.h"
#include "rose/position.h"
#include "rose/util/pext.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  template <typename T> auto MoveList::write(typename T::Mask16 mask, T v) -> void {
    const usize count = std::popcount(mask);
    rose_assert(len + count < capacity());
    vec::compressstore16(data.data() + len, mask, v);
    len += count;
  }

  template <typename T> auto MoveList::write2(typename T::Mask32 mask, T v) -> void {
    const usize count = std::popcount(mask) * 2;
    rose_assert(len + count < capacity());
    vec::compressstore32(data.data() + len, mask, v);
    len += count;
  }

  template <typename T> auto MoveList::write4(typename T::Mask64 mask, T v) -> void {
    const usize count = std::popcount(mask) * 4;
    rose_assert(len + count < capacity());
    vec::compressstore64(data.data() + len, mask, v);
    len += count;
  }

  auto MoveList::write4(u64 moves) -> void {
    const usize count = 4;
    rose_assert(len + count < capacity());
    std::memcpy(data.data() + len, &moves, sizeof(u64));
    len += count;
  }

  auto MoveGen::calculatePinInfo() -> void {
    const Color active_color = m_position.activeColor();

    std::tie(m_king_ray_coords, m_king_ray_valid) = geometry::superpieceRays(m_king_sq);
    m_king_ray_places = vec::permute8(m_king_ray_coords, m_position.board().z);

    const u64 occupied = m_king_ray_places.nonzero8() & m_king_ray_valid & geometry::non_horse_attack_mask;
    const u64 color = m_king_ray_places.msb8();
    const u64 enemy_pieces = (color ^ active_color.toBitboard()) & occupied;

    // Closest blockers (color doesn't matter, because we want to use this to detect pinned en passant pawns as well).
    const u64 potentially_pinned = occupied & geometry::superpieceAttacks(occupied, m_king_ray_valid);

    // Find all enemy sliders with the correct attacks for the rays they're on
    const u64 maybe_attacking = enemy_pieces & (m_king_ray_places & geometry::superpieceAttackerMask(active_color)).nonzero8();
    // Second closest blockers
    const u64 not_closest = occupied & ~potentially_pinned;
    m_pin_raymasks = geometry::superpieceAttacks(not_closest, m_king_ray_valid);
    const u64 potential_attackers = not_closest & m_pin_raymasks;
    // Second closest blockers that are of the correct type to be pinning attackers.
    const u64 attackers = maybe_attacking & potential_attackers;

    // A closest blocker is pinned if it has a valid pinning attacker.
    const u16 has_attacker_vecmask = v128::from64(attackers).nonzero8();
    m_pinned = vec::mask8(has_attacker_vecmask, v128::from64(potentially_pinned)).to64();
    m_pinned_friendly = m_pinned & ~enemy_pieces;

    // Convert from list of squares to piecemask
    const int pinned_count = std::popcount(m_pinned);
    const v128 pinned_coord = vec::compress8(m_pinned, m_king_ray_coords).to128();
    m_pinned_piece_mask = vec::findset8(pinned_coord, pinned_count, m_position.pieceListSq(active_color).x);
  }

  auto MoveGen::generateSubsetNorm(MoveList &moves, const Wordboard &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void {
    const Color active_color = m_position.activeColor();
    for (; bitboard != 0; bitboard &= bitboard - 1) {
      const Square sq{narrow_cast<u8>(std::countr_zero(bitboard))};
      const u16 mask = piecemask & attack_table.r[sq.raw];
      if (mask) {
        const v256 dest = v256::broadcast16(static_cast<u16>(sq.raw) << 6);
        moves.write(mask, srcs | dest);
      }
    }
  }

  auto MoveGen::generateSubsetCaps(MoveList &moves, const Wordboard &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void {
    const Color active_color = m_position.activeColor();
    for (; bitboard != 0; bitboard &= bitboard - 1) {
      const Square sq{narrow_cast<u8>(std::countr_zero(bitboard))};
      const u16 mask = piecemask & attack_table.r[sq.raw];
      if (mask) {
        const v256 dest = v256::broadcast16(static_cast<u16>(MoveFlags::capture) | (static_cast<u16>(sq.raw) << 6));
        moves.write(mask, srcs | dest);
      }
    }
  }

  auto MoveGen::generateSubsetPCap(MoveList &moves, const Wordboard &attack_table, u64 bitboard, u16 piecemask) -> void {
    const Color active_color = m_position.activeColor();
    for (; bitboard != 0; bitboard &= bitboard - 1) {
      const Square sq{narrow_cast<u8>(std::countr_zero(bitboard))};
      u16 mask = piecemask & attack_table.r[sq.raw];
      for (; mask != 0; mask &= mask - 1) {
        const int pindex = std::countr_zero(mask);
        const Square src = m_position.pieceListSq(active_color).m[pindex];
        const u16 base_move = (static_cast<u16>(sq.raw) << 6) | src.raw;
        constexpr u64 promos = static_cast<u64>(MoveFlags::cap_promo_q) | (static_cast<u64>(MoveFlags::cap_promo_n) << 16) |
                               (static_cast<u64>(MoveFlags::cap_promo_r) << 32) | (static_cast<u64>(MoveFlags::cap_promo_b) << 48);
        moves.write4(promos + 0x0001000100010001 * base_move);
      }
    }
  }

  auto MoveGen::generateMovesNoCheckers(MoveList &moves, const Position &position, Square king_sq) -> void {
    const Color active_color = position.activeColor();

    const u64 empty = position.board().getEmptyBitboard();
    const u64 enemy = position.board().getColorBitboard(active_color.invert());

    const Wordboard &attack_table = position.attackTable(active_color);
    const u64 active = position.attackTable(active_color).getAttackedBitboard();
    const u64 danger = position.attackTable(active_color.invert()).getAttackedBitboard();

    const u16 valid_plist = position.pieceListType(active_color).x.nonzero8();
    const u16 king_mask = position.pieceListType(active_color).maskEq(PieceType::k);
    const u16 pawn_mask = position.pieceListType(active_color).maskEq(PieceType::p);

    const auto pawn_info = pawns::pawnShifts(active_color);

    const v256 srcs = vec::zext8to16(m_position.pieceListSq(active_color).x);

    // TODO: Pinned masking

    // Unprotected captures
    generateSubsetCaps(moves, attack_table, srcs, active & enemy & ~danger, valid_plist & ~pawn_mask);
    // Capture-with-promotion
    generateSubsetPCap(moves, attack_table, active & enemy & pawn_info.promo_zone, pawn_mask);
    // Enpassant
    if (position.enpassant().isValid()) {
      const Square sq = position.enpassant();
      const u16 mask = attack_table.r[sq.raw] & pawn_mask;
      const v256 dest = v256::broadcast16(static_cast<u16>(MoveFlags::enpassant) | (static_cast<u16>(sq.raw) << 6));
      moves.write(mask, srcs | dest);
    }
    // Pawn captures
    generateSubsetCaps(moves, attack_table, srcs, active & enemy & pawn_info.non_promo_dest, pawn_mask);
    // Protected captures
    generateSubsetCaps(moves, attack_table, srcs, active & enemy & danger, valid_plist & ~pawn_mask & ~king_mask);
    // Unprotected non-pawn quiets
    generateSubsetNorm(moves, attack_table, srcs, active & empty & ~danger, valid_plist & ~pawn_mask);
    // Protected non-pawn quiets
    generateSubsetNorm(moves, attack_table, srcs, active & empty & danger, valid_plist & ~pawn_mask & ~king_mask);
    // Do pawns
    {
      const u64 bb = position.board().bitboardFor<PieceType::p>(active_color);
      const auto pawn_empty = pawns::pawnDestinationEmpty(active_color, empty);
      const auto pawn_moves = pawns::pawnMoves(active_color);

      const u64 pawn_normal = bb & pawn_empty.normal_move;
      const u64 pawn_double = bb & pawn_empty.double_move;

      // Promotions
      {
        const u8 mask = static_cast<u8>(pawn_normal >> pawn_info.promotable_shift);
        moves.write4(mask, pawn_moves.promotions);
      }
      // Relative rank 3-6
      {
        const u32 mask = static_cast<u32>(pawn_normal >> pawn_info.normal_shift);
        moves.write(mask, pawn_moves.normal_moves);
      }
      // Relative rank 2
      {
        const u8 normal_mask = static_cast<u8>(pawn_normal >> pawn_info.second_rank_shift);
        const u8 double_mask = static_cast<u8>(pawn_double >> pawn_info.second_rank_shift);
        const u16 mask = (static_cast<u16>(double_mask) << 8) | normal_mask;
        moves.write(mask, pawn_moves.double_moves);
      }
    }
  }

  auto MoveGen::generateMoves(MoveList &moves) -> void {
    const Color active_color = m_position.activeColor();
    const Square king_sq = m_position.kingSq(active_color);

    // HACK: Pseudolegal movegen
    generateMovesNoCheckers(moves, m_position, king_sq);

    // const u16 checkers = m_position.attackTable(active_color.invert()).r[king_sq.raw];
    // const int checkers_count = std::popcount(checkers);
    //
    // switch (checkers_count) {
    // case 0:
    //   return generateMovesNoCheckers(moves, m_position, king_sq);
    // case 1:
    //   return generateMovesOneCheckers(moves, m_position, king_sq, checkers);
    // default:
    //   return generateMovesTwoCheckers(moves, m_position, king_sq, checkers);
    // }
  }

  static auto generateMovesOneCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void {}

  static auto writeKingMovesWithCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void;

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

} // namespace rose
