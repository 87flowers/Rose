#include "rose/movegen.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <tuple>

#include "rose/common.h"
#include "rose/geometry.h"
#include "rose/pawns.h"
#include "rose/position.h"
#include "rose/rays.h"
#include "rose/util/assert.h"
#include "rose/util/pext.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  template <typename T> auto MoveList::write(typename T::Mask16 mask, T v) -> void {
    rose_assert(len + sizeof(T) / sizeof(Move) < cap);
    const T y = vec::compress16(mask, v);
    std::memcpy(data.data() + len, &y, sizeof(T));
    len += std::popcount(mask);
  }

  template <typename T> auto MoveList::write2(typename T::Mask32 mask, T v) -> void {
    rose_assert(len + sizeof(T) / sizeof(Move) < cap);
    const T y = vec::compress32(mask, v);
    std::memcpy(data.data() + len, &y, sizeof(T));
    len += std::popcount(mask) * 2;
  }

  template <typename T> auto MoveList::write4(typename T::Mask64 mask, T v) -> void {
    rose_assert(len + sizeof(T) / sizeof(Move) < cap);
    const T y = vec::compress64(mask, v);
    std::memcpy(data.data() + len, &y, sizeof(T));
    len += std::popcount(mask) * 4;
  }

  auto MoveList::write4(u64 moves) -> void {
    const usize count = 4;
    rose_assert(len + count < capacity());
    std::memcpy(data.data() + len, &moves, sizeof(u64));
    len += count;
  }

  PrecompMoveGenInfo::PrecompMoveGenInfo(const Position &position) {
    const auto between = [](Square a, Square b) -> u64 {
      if (!a.isValid() || !b.isValid())
        return 0;

      u64 result = 0;
      const u8 start = std::min(a.raw, b.raw);
      const u8 end = std::max(a.raw, b.raw);
      for (u8 i = start; i <= end; i++) {
        result |= static_cast<u64>(1) << i;
      }
      return result;
    };

    aside_rook[0] = between(position.rookInfo(Color::white).aside, Square::parse("d1").value());
    aside_rook[1] = between(position.rookInfo(Color::black).aside, Square::parse("d8").value());
    aside_king[0] = between(position.kingSq(Color::white), Square::parse("c1").value());
    aside_king[1] = between(position.kingSq(Color::black), Square::parse("c8").value());
    hside_rook[0] = between(position.rookInfo(Color::white).hside, Square::parse("f1").value());
    hside_rook[1] = between(position.rookInfo(Color::black).hside, Square::parse("f8").value());
    hside_king[0] = between(position.kingSq(Color::white), Square::parse("g1").value());
    hside_king[1] = between(position.kingSq(Color::black), Square::parse("g8").value());
  }

  auto MoveGen::calculatePinInfo() -> std::tuple<Wordboard, u64> {
    const Color active_color = m_position.activeColor();
    const Square sq = m_position.kingSq(active_color);

    const auto [ray_coords, ray_valid] = geometry::superpieceRays(sq);
    const v512 ray_places = vec::permute8(ray_coords, m_position.board().z);

    const v512 perm = geometry::superpieceInverseRays(sq);

    const u64 blockers = ray_places.nonzero8() & geometry::non_horse_attack_mask;
    const u64 color = ray_places.msb8();
    const u64 enemy = (color ^ active_color.toBitboard()) & blockers;

    // Closest blockers (color doesn't matter, because we want to use this to detect pinned en passant pawns as well).
    const u64 potentially_pinned = blockers & geometry::superpieceAttacks(blockers, ray_valid);

    // Find all enemy sliders with the correct attacks for the rays they're on
    const u64 maybe_attacking = enemy & geometry::slidersFromRays(ray_places);
    // Second closest blockers
    const u64 not_closest = blockers & ~potentially_pinned;
    const u64 pin_raymasks = geometry::superpieceAttacks(not_closest, ray_valid) & geometry::non_horse_attack_mask;
    const u64 potential_attackers = not_closest & pin_raymasks;
    // Second closest blockers that are of the correct type to be pinning attackers.
    const u64 attackers = maybe_attacking & potential_attackers;

    // A closest blocker is pinned if it has a valid pinning attacker.
    const u16 has_attacker_vecmask = v128::from64(attackers).nonzero8();
    const u64 pinned = vec::mask8(has_attacker_vecmask, v128::from64(potentially_pinned)).to64() & ~enemy;

    // Translate to valid move rays
    const v512 pinned_ids = vec::mask8(pin_raymasks, vec::lanebroadcast8to64(vec::mask8(pinned, ray_places)));
    const v512 board_layout = vec::permute8_mz(~perm.msb8(), perm, pinned_ids);

    // Convert from list of squares to piecemask
    const int pinned_count = std::popcount(pinned);
    const v128 pinned_coord = vec::compress8(pinned, ray_coords).to128();
    const u16 piece_mask = vec::findset8(pinned_coord, pinned_count, m_position.pieceListSq(active_color).x());

    // Generate attack table mask
    const v512 ones = v512::broadcast16(1);
    const u64 valid_ids = board_layout.nonzero8();
    const v512 masked_ids = board_layout & v512::broadcast8(0xF);
    const v512 bits0 = vec::shl16_mz(static_cast<u32>(valid_ids), ones, vec::zext8to16(masked_ids.to256()));
    const v512 bits1 = vec::shl16_mz(static_cast<u32>(valid_ids >> 32), ones, vec::zext8to16(vec::extract256<1>(masked_ids)));
    const v512 at_mask0 = v512::broadcast16(~piece_mask) | bits0;
    const v512 at_mask1 = v512::broadcast16(~piece_mask) | bits1;

    const u64 pinned_bb = board_layout.nonzero8();

    return {{at_mask0, at_mask1}, pinned_bb};
  }

  auto MoveGen::generateSubsetNorm(MoveList &moves, const std::array<u16, 64> &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void {
    const Color active_color = m_position.activeColor();
    for (; bitboard != 0; bitboard &= bitboard - 1) {
      const Square sq{narrow_cast<u8>(std::countr_zero(bitboard))};
      const u16 mask = piecemask & attack_table[sq.raw];
      if (mask) {
        const v256 dest = v256::broadcast16(static_cast<u16>(sq.raw) << 6);
        moves.write(mask, srcs | dest);
      }
    }
  }

  auto MoveGen::generateSubsetCaps(MoveList &moves, const std::array<u16, 64> &attack_table, v256 srcs, u64 bitboard, u16 piecemask) -> void {
    const Color active_color = m_position.activeColor();
    for (; bitboard != 0; bitboard &= bitboard - 1) {
      const Square sq{narrow_cast<u8>(std::countr_zero(bitboard))};
      const u16 mask = piecemask & attack_table[sq.raw];
      if (mask) {
        const v256 dest = v256::broadcast16(static_cast<u16>(MoveFlags::cap_normal) | (static_cast<u16>(sq.raw) << 6));
        moves.write(mask, srcs | dest);
      }
    }
  }

  auto MoveGen::generateSubsetPCap(MoveList &moves, const std::array<u16, 64> &attack_table, u64 bitboard, u16 piecemask) -> void {
    const Color active_color = m_position.activeColor();
    for (; bitboard != 0; bitboard &= bitboard - 1) {
      const Square sq{narrow_cast<u8>(std::countr_zero(bitboard))};
      u16 mask = piecemask & attack_table[sq.raw];
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

  template <bool king_moves> auto MoveGen::generateMovesTo(MoveList &moves, Square king_sq, u64 valid_destinations, bool can_ep) -> void {
    const Position &position = m_position;
    const Color active_color = position.activeColor();

    const u64 empty = position.board().getEmptyBitboard();
    const u64 enemy = position.board().getColorBitboard(active_color.invert());

    const auto [pin_atmask, pinned_bb] = calculatePinInfo();
    const auto attack_table = (position.attackTable(active_color) & pin_atmask).toMailbox();

    const u16 valid_plist = position.pieceListType(active_color).valid();
    const u16 king_mask = 1;
    const u16 pawn_mask = position.pieceListType(active_color).maskEq(PieceType::p);
    const u16 non_pawn_mask = valid_plist & ~pawn_mask & ~king_mask;

    const u64 pawn_active = position.attackTable(active_color).getBitboardFor(pawn_mask) & valid_destinations;
    const u64 non_pawn_active = position.attackTable(active_color).getBitboardFor(non_pawn_mask) & valid_destinations;
    const u64 king_active = position.attackTable(active_color).getBitboardFor(king_mask) & valid_destinations;
    const u64 danger = position.attackTable(active_color.invert()).getAttackedBitboard();

    const auto pawn_info = pawns::pawnShifts(active_color);

    const v256 srcs = vec::zext8to16(position.pieceListSq(active_color).x());

    // Capture-with-promotion
    generateSubsetPCap(moves, attack_table, pawn_active & enemy & pawn_info.promo_zone, pawn_mask);
    // Enpassant
    if (position.enpassant().isValid() && can_ep) {
      const Square sq = position.enpassant();
      const u16 mask = attack_table[sq.raw] & pawn_mask;
      if (mask != 0) {
        const Square victim{static_cast<u8>(sq.raw + (active_color.raw ? 8 : -8))};
        if (std::popcount(mask) > 1 || victim.rank() != king_sq.rank() || [&] {
              // Detect clearance pins
              const Square my_piece = position.pieceListSq(active_color).m[std::countr_zero(mask)];
              const int direction = victim.file() < king_sq.file() ? -1 : 1;
              Square test_sq = king_sq;
              const auto advance = [&] { test_sq.raw += direction; };
              advance();
              for (; test_sq.rank() == king_sq.rank(); advance()) {
                const Place p = position.board().read(test_sq);
                if (p.isEmpty() || test_sq == victim || test_sq == my_piece)
                  continue;
                return p.color() == active_color || (p.ptype() != PieceType::r && p.ptype() != PieceType::q);
              }
              return true;
            }()) {
          const v256 dest = v256::broadcast16(static_cast<u16>(MoveFlags::enpassant) | (static_cast<u16>(sq.raw) << 6));
          moves.write(mask, srcs | dest);
        }
      }
    }
    // Pawn captures
    generateSubsetCaps(moves, attack_table, srcs, pawn_active & enemy & pawn_info.non_promo_dest, pawn_mask);
    // Non-pawn captures
    generateSubsetCaps(moves, attack_table, srcs, non_pawn_active & enemy, non_pawn_mask);
    // King captures
    if constexpr (king_moves) {
      generateSubsetCaps(moves, attack_table, srcs, king_active & enemy & ~danger, king_mask);
    }
    // Castling
    if constexpr (king_moves) {
#define IS_CLEAR(bb, x) ((~(bb) & m_precomp_info.x[active_color.toIndex()]) == 0)
      const RookInfo rook_info = position.rookInfo(active_color);
      const u64 king_bb = static_cast<u64>(1) << king_sq.raw;
      if (rook_info.aside.isValid()) {
        rose_assert((position.board().read(rook_info.aside).raw & 0xF0) == Place::fromColorAndPtypeAndId(active_color, PieceType::r, 0).raw);
        const u64 rook_bb = static_cast<u64>(1) << rook_info.aside.raw;
        const u64 clear = empty | king_bb | rook_bb;
        if (IS_CLEAR(clear, aside_rook) && IS_CLEAR(~danger & clear, aside_king) && !(rook_bb & pinned_bb)) {
          moves.push_back(Move::make(king_sq, rook_info.aside, MoveFlags::castle_aside));
        }
      }
      if (rook_info.hside.isValid()) {
        rose_assert((position.board().read(rook_info.hside).raw & 0xF0) == Place::fromColorAndPtypeAndId(active_color, PieceType::r, 0).raw);
        const u64 rook_bb = static_cast<u64>(1) << rook_info.hside.raw;
        const u64 clear = empty | king_bb | rook_bb;
        if (IS_CLEAR(clear, hside_rook) && IS_CLEAR(~danger & clear, hside_king) && !(rook_bb & pinned_bb)) {
          moves.push_back(Move::make(king_sq, rook_info.hside, MoveFlags::castle_hside));
        }
      }
#undef IS_CLEAR
    }
    // Non-pawn quiets
    generateSubsetNorm(moves, attack_table, srcs, non_pawn_active & empty, non_pawn_mask);
    // King quiets
    if constexpr (king_moves) {
      generateSubsetNorm(moves, attack_table, srcs, king_active & empty & ~danger, king_mask);
    }
    // Do pawns
    {
      const u64 pinned_pawns = pinned_bb & ~(0x0101010101010101 << king_sq.file());

      const u64 bb = position.board().bitboardFor<PieceType::p>(active_color) & ~pinned_pawns;
      const auto pawn_empty = pawns::pawnDestinationEmpty(active_color, empty, valid_destinations);
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
        constexpr int normal_shift = 16;
        const u32 mask = static_cast<u32>(pawn_normal >> normal_shift);
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

  auto MoveGen::generateMovesNoCheckers(MoveList &moves, Square king_sq) -> void {
    generateMovesTo<true>(moves, king_sq, ~static_cast<u64>(0), true);
  }

  static auto generateKingMoves(MoveList &moves, const Position &position, Square king_sq, u64 valid_destinations) -> void;

  auto MoveGen::generateMovesOneChecker(MoveList &moves, Square king_sq, u16 checkers) -> void {
    const Position &position = m_position;
    const Color active_color = position.activeColor();

    const int checker_id = std::countr_zero(checkers);
    const PieceType checker_ptype = position.pieceListType(active_color.invert()).m[checker_id];
    const Square checker_sq = position.pieceListSq(active_color.invert()).m[checker_id];

    const u64 valid_destinations = rays::inclusive(king_sq, checker_sq);
    const u64 checker_ray = checker_ptype.isSlider() ? rays::infinite_exclusive(king_sq, checker_sq) : 0;

    generateMovesTo<false>(moves, king_sq, valid_destinations, checker_ptype == PieceType::p);
    generateKingMoves(moves, m_position, king_sq, ~checker_ray);
  }

  auto MoveGen::generateMovesTwoCheckers(MoveList &moves, Square king_sq, u16 checkers) -> void {
    const Position &position = m_position;
    const Color active_color = position.activeColor();

    u64 checker_rays = 0;
    for (; checkers != 0; checkers &= checkers - 1) {
      const int checker_id = std::countr_zero(checkers);
      const PieceType checker_ptype = position.pieceListType(active_color.invert()).m[checker_id];
      const Square checker_sq = position.pieceListSq(active_color.invert()).m[checker_id];
      checker_rays |= checker_ptype.isSlider() ? rays::infinite_exclusive(king_sq, checker_sq) : 0;
    }

    generateKingMoves(moves, m_position, king_sq, ~checker_rays);
  }

  auto MoveGen::generateMoves(MoveList &moves) -> void {
    const Color active_color = m_position.activeColor();
    const Square king_sq = m_position.kingSq(active_color);

    const u16 checkers = m_position.attackTable(active_color.invert()).read(king_sq);
    const int checkers_count = std::popcount(checkers);
    switch (checkers_count) {
    case 0:
      return generateMovesNoCheckers(moves, king_sq);
    case 1:
      return generateMovesOneChecker(moves, king_sq, checkers);
    default:
      return generateMovesTwoCheckers(moves, king_sq, checkers);
    }
  }

  forceinline static auto generateKingMoves(MoveList &moves, const Position &position, Square king_sq, u64 valid_destinations) -> void {
    const Color active_color = position.activeColor();

    const u64 empty = position.board().getEmptyBitboard();
    const u64 enemy = position.board().getColorBitboard(active_color.invert());

    const u16 king_mask = 1;
    const u64 active = position.attackTable(active_color).getBitboardFor(king_mask) & valid_destinations;
    const u64 danger = position.attackTable(active_color.invert()).getAttackedBitboard();

    const auto write_king = [&](u64 bitboard, MoveFlags mf) {
      for (; bitboard != 0; bitboard &= bitboard - 1) {
        const Square dest{narrow_cast<u8>(std::countr_zero(bitboard))};
        moves.push_back(Move::make(king_sq, dest, mf));
      }
    };

    // Unprotected captures
    write_king(active & enemy & ~danger, MoveFlags::cap_normal);
    // Unprotected quiets
    write_king(active & empty & ~danger, MoveFlags::normal);
  }

} // namespace rose
