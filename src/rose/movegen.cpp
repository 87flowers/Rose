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

  auto MoveGen::calculatePinInfo() -> std::tuple<std::array<v512, 2>, u64> {
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
    const u16 piece_mask = vec::findset8(pinned_coord, pinned_count, m_position.pieceListSq(active_color).x);

    // Generate attack table mask
    const v512 ones = v512::broadcast16(1);
    const u64 valid_ids = board_layout.nonzero8();
    const v512 masked_ids = board_layout & v512::broadcast8(0xF);
    const v512 bits0 = vec::shl16_mz(static_cast<u32>(valid_ids), ones, vec::zext8to16(masked_ids.to256()));
    const v512 bits1 = vec::shl16_mz(static_cast<u32>(valid_ids >> 32), ones, vec::zext8to16(vec::extract256<1>(masked_ids)));
    const v512 at_mask0 = v512::broadcast16(~piece_mask) | bits0;
    const v512 at_mask1 = v512::broadcast16(~piece_mask) | bits1;

    const u64 pinned_bb = vec::bitshuffle(v512::broadcast64(pinned), perm);

    return {{at_mask0, at_mask1}, pinned_bb};
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

  template <bool king_moves> auto MoveGen::generateMovesTo(MoveList &moves, Square king_sq, u64 valid_destinations, PieceType checker_ptype) -> void {
    const Position &position = m_position;
    const Color active_color = position.activeColor();

    const u64 empty = position.board().getEmptyBitboard();
    const u64 enemy = position.board().getColorBitboard(active_color.invert());

    const auto [pin_atmask, pinned_bb] = calculatePinInfo();
    Wordboard attack_table = position.attackTable(active_color);
    attack_table.z[0] &= pin_atmask[0];
    attack_table.z[1] &= pin_atmask[1];

    const u64 active = position.attackTable(active_color).getAttackedBitboard() & valid_destinations;
    const u64 danger = position.attackTable(active_color.invert()).getAttackedBitboard();

    const u16 valid_plist = position.pieceListType(active_color).x.nonzero8() & ~(king_moves ? 0 : 1);
    const u16 king_mask = 1;
    const u16 pawn_mask = position.pieceListType(active_color).maskEq(PieceType::p);

    const auto pawn_info = pawns::pawnShifts(active_color);

    const v256 srcs = vec::zext8to16(position.pieceListSq(active_color).x);

    // TODO: Pinned masking

    // Unprotected captures
    generateSubsetCaps(moves, attack_table, srcs, active & enemy & ~danger, valid_plist & ~pawn_mask);
    // Capture-with-promotion
    generateSubsetPCap(moves, attack_table, active & enemy & pawn_info.promo_zone, pawn_mask);
    // Enpassant
    if (position.enpassant().isValid() && (king_moves || checker_ptype == PieceType::p)) {
      const Square sq = position.enpassant();
      const u16 mask = attack_table.r[sq.raw] & pawn_mask;
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
                const Place p = position.board().m[test_sq.raw];
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
    generateSubsetCaps(moves, attack_table, srcs, active & enemy & pawn_info.non_promo_dest, pawn_mask);
    // Protected captures
    generateSubsetCaps(moves, attack_table, srcs, active & enemy & danger, valid_plist & ~pawn_mask & ~king_mask);
    // Castling
    // TODO: Handle pinned rook
    if constexpr (king_moves) {
#define IS_CLEAR(bb, x) ((~(bb) & m_precomp_info.x[active_color.toIndex()]) == 0)
      const RookInfo rook_info = position.rookInfo(active_color);
      if (rook_info.aside.isValid()) {
        rose_assert(position.board().m[rook_info.aside.raw].raw & 0xF0 == Place::fromColorAndPtypeAndId(active_color, PieceType::r, 0).raw);
        const u64 clear = empty | (static_cast<u64>(1) << king_sq.raw) | (static_cast<u64>(1) << rook_info.aside.raw);
        if (IS_CLEAR(clear, aside_rook) && IS_CLEAR(~danger & clear, aside_king)) {
          moves.push_back(Move::make(king_sq, rook_info.aside, MoveFlags::castle_aside));
        }
      }
      if (rook_info.hside.isValid()) {
        rose_assert(position.board().m[rook_info.hside.raw].raw & 0xF0 == Place::fromColorAndPtypeAndId(active_color, PieceType::r, 0).raw);
        const u64 clear = empty | (static_cast<u64>(1) << king_sq.raw) | (static_cast<u64>(1) << rook_info.hside.raw);
        if (IS_CLEAR(clear, hside_rook) && IS_CLEAR(~danger & clear, hside_king)) {
          moves.push_back(Move::make(king_sq, rook_info.hside, MoveFlags::castle_hside));
        }
      }
#undef IS_CLEAR
    }
    // Unprotected non-pawn quiets
    generateSubsetNorm(moves, attack_table, srcs, active & empty & ~danger, valid_plist & ~pawn_mask);
    // Protected non-pawn quiets
    generateSubsetNorm(moves, attack_table, srcs, active & empty & danger, valid_plist & ~pawn_mask & ~king_mask);
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
    generateMovesTo<true>(moves, king_sq, ~static_cast<u64>(0), PieceType::none);
  }

  template <usize checkers_count>
  static auto writeKingMovesWithCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void;

  auto MoveGen::generateMovesOneChecker(MoveList &moves, Square king_sq, u16 checkers) -> void {
    const int checker_index = std::countr_zero(checkers);
    const PieceType checker_ptype = m_position.pieceListType(m_position.activeColor().invert()).m[checker_index];
    const Square checker_sq = m_position.pieceListSq(m_position.activeColor().invert()).m[checker_index];
    generateMovesTo<false>(moves, king_sq, checker_ptype == PieceType::n ? checker_sq.toBitboard() : rays::calcRayTo(king_sq, checker_sq),
                           checker_ptype);
    writeKingMovesWithCheckers<1>(moves, m_position, king_sq, checkers);
  }

  auto MoveGen::generateMovesTwoCheckers(MoveList &moves, Square king_sq, u16 checkers) -> void {
    writeKingMovesWithCheckers<2>(moves, m_position, king_sq, checkers);
  }

  auto MoveGen::generateMoves(MoveList &moves) -> void {
    const Color active_color = m_position.activeColor();
    const Square king_sq = m_position.kingSq(active_color);

    const u16 checkers = m_position.attackTable(active_color.invert()).r[king_sq.raw];
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

  template <usize checker_count>
  forceinline static auto writeKingMovesWithCheckers(MoveList &moves, const Position &position, Square king_sq, u16 checkers) -> void {
    const Color active_color = position.activeColor();
    const Wordboard &attack_table = position.attackTable(position.activeColor().invert());

    const auto [king_rays, rays_valid] = geometry::superpieceRays(king_sq);
    const auto [king_leaps, leaps_valid] = geometry::adjacents(king_sq);
    const auto king_leaps16 = vec::zext8to16_lo(king_leaps);

    const v128 places = vec::permute8(v512::from128(king_leaps), position.board().z).to128();
    const u16 occupied = places.nonzero8();
    const u16 color = places.msb8();
    const u16 friendly = (color ^ ~active_color.toBitboard()) & occupied;

    const u64 at_empty = vec::concatlo64(attack_table.z[0].zero16(), attack_table.z[1].zero16());
    const u16 no_attackers = vec::bitshuffle_m(leaps_valid, v128::from64(at_empty), king_leaps);

    u8 destinations = static_cast<u8>(leaps_valid & ~friendly & no_attackers);

    u8 additional_checks = 0;
    for (usize i = 0; i < checker_count; i++, checkers &= checkers - 1) {
      const int checker_index = std::countr_zero(checkers);
      const PieceType checker_ptype = position.pieceListType(active_color.invert()).m[checker_index];
      const Square checker_sq = position.pieceListSq(active_color.invert()).m[checker_index];

      if (checker_ptype.isSlider()) {
        // LSB -> MSB : n, ne, e, se, s, sw, w, nw
        constexpr std::array<u8, 3> valid_mask{{0b10101010, 0b01010101, 0b11111111}};
        static_assert(PieceType::b == 0b101 && PieceType::r == 0b110 && PieceType::q == 0b111);

        const int direction = std::countr_zero(std::rotl(rays_valid & vec::eq8(king_rays, v512::broadcast8(checker_sq.raw)), 32)) / 8;
        additional_checks |= (1 << direction) & valid_mask[checker_ptype.raw - PieceType::b];
      }
    }

    destinations &= ~additional_checks;

    const v128 write_vector =
        vec::shl16(king_leaps16, 6) | v128::broadcast16(king_sq.raw) | vec::mask16(occupied, v128::broadcast16(static_cast<u16>(MoveFlags::capture)));
    moves.write(destinations, write_vector);
  }

} // namespace rose
