#include "rose/movegen.hpp"

#include "rose/bitboard.hpp"
#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/pawns.hpp"
#include "rose/position.hpp"
#include "rose/util/static_vector.hpp"

#include <bit>

namespace rose {

  inline static constexpr auto backrank_ray(Square a, Square b) -> Bitboard {
    return a.raw > b.raw ? Bitboard {(a.to_bitboard().raw << 1) - b.to_bitboard().raw} : Bitboard {(b.to_bitboard().raw << 1) - a.to_bitboard().raw};
  }

  template<typename M, typename T>
  auto MoveList::write(M mask, T v) -> void {
    const usize count = std::popcount(mask);
    rose_assert(len + count < capacity());
    for (int i = 0; i < count; i++, mask &= mask - 1)
      std::memcpy(data.data() + len + i, reinterpret_cast<char*>(&v.raw) + std::countr_zero(mask) * sizeof(u16), sizeof(u16));
    len += count;
  }

  template<typename M, typename T>
  auto MoveList::write2(M mask, T v) -> void {
    const usize count = std::popcount(mask);
    rose_assert(len + count * 2 < capacity());
    for (int i = 0; i < count; i++, mask &= mask - 1)
      std::memcpy(data.data() + len + i * 2, reinterpret_cast<char*>(&v.raw) + std::countr_zero(mask) * sizeof(u16) * 2, sizeof(u16) * 2);
    len += count * 2;
  }

  template<typename M, typename T>
  auto MoveList::write4(M mask, T v) -> void {
    const usize count = std::popcount(mask);
    rose_assert(len + count * 4 < capacity());
    for (int i = 0; i < count; i++, mask &= mask - 1)
      std::memcpy(data.data() + len + i * 4, reinterpret_cast<char*>(&v.raw) + std::countr_zero(mask) * sizeof(u16) * 4, sizeof(u16) * 4);
    len += count * 4;
  }

  template<MoveFlags mf>
  auto MoveGen::write_moves(MoveList& moves, const std::array<PieceMask, 64>& at, u16x16 srcs, Bitboard bb, PieceMask pm) -> void {
    for (Square to : bb) {
      const PieceMask mask = pm & at[to.raw];
      if (!mask.is_empty()) {
        const u16x16 dest = u16x16::splat(static_cast<u16>(mf) | (static_cast<u16>(to.raw) << 6));
        moves.write(mask.raw, srcs | dest);
      }
    }
  }

  auto MoveGen::write_cap_promo(MoveList& moves, const std::array<PieceMask, 64>& at, Bitboard bb, PieceMask pm) -> void {
    const Color stm = m_position.stm();
    for (Square to : bb) {
      const PieceMask mask = pm & at[to.raw];
      for (PieceId pi : mask) {
        const Square from = m_position.piece_list_sq(stm)[pi];
        moves.push_back(Move::make(from, to, MoveFlags::cap_promo_q));
        moves.push_back(Move::make(from, to, MoveFlags::cap_promo_n));
        moves.push_back(Move::make(from, to, MoveFlags::cap_promo_r));
        moves.push_back(Move::make(from, to, MoveFlags::cap_promo_b));
      }
    }
  }

  auto MoveGen::generate_moves_no_checkers(MoveList& moves, Square king_sq) -> void {
    const Bitboard valid_destinations = ~Bitboard {};

    const Position& position = m_position;
    const Color stm = position.stm();

    const Bitboard empty = position.board().empty_bitboard();
    const Bitboard enemy = position.board().color_bitboard(!stm);

    const std::array<PieceMask, 64> at = position.attack_table(stm).to_mailbox();

    const PieceMask king_mask = PieceMask::king();
    const PieceMask pawn_mask = position.piece_mask_for<PieceType::p>(stm);
    const PieceMask nonpawn_mask = ~pawn_mask;

    const Bitboard pawn_active = position.attack_table(stm).bitboard_for(pawn_mask);
    const Bitboard nonpawn_active = position.attack_table(stm).bitboard_for(~pawn_mask);
    const Bitboard king_active = position.attack_table(stm).bitboard_for(king_mask);
    const Bitboard danger = position.attack_table(!stm).bitboard_any();

    const auto pawn_info = pawns::pawn_shifts(stm);

    const u16x16 srcs = position.piece_list_sq(stm).to_vector().convert<u16>();

    // Capture-with-promotion
    write_cap_promo(moves, at, pawn_active & enemy & pawn_info.promo_zone, pawn_mask);
    // En passant
    if (const Square ep = position.enpassant(); ep.is_valid()) {
      const PieceMask attackers = at[ep.raw] & pawn_mask;
      if (!attackers.is_empty()) {
        const Square victim = Square::from_file_and_rank(ep.file(), stm == Color::white ? 4 : 3);
        if (attackers.popcount() > 1 || victim.rank() != king_sq.rank() || [&] {
              // Detect clearance pins
              const Square my_pawn = position.piece_list_sq(stm)[attackers.lsb()];
              const int direction = victim.file() < king_sq.file() ? -1 : 1;
              for (Square test_sq {static_cast<u8>(king_sq.raw + direction)}; test_sq.rank() == king_sq.rank(); test_sq.raw += direction) {
                const Place p = position.board()[test_sq];
                if (p.is_empty() || test_sq == victim || test_sq == my_pawn)
                  continue;
                return p.color() == stm || (p.ptype() != PieceType::r && p.ptype() != PieceType::q);
              }
              return true;
            }()) {
          const u16x16 dest = u16x16::splat(static_cast<u16>(MoveFlags::enpassant) | (static_cast<u16>(ep.raw) << 6));
          moves.write(attackers.raw, srcs | dest);
        }
      }
    }
    // Pawn captures
    write_moves<MoveFlags::cap_normal>(moves, at, srcs, pawn_active & enemy & pawn_info.non_promo_dest, pawn_mask);
    // Non-pawn captures
    write_moves<MoveFlags::cap_normal>(moves, at, srcs, nonpawn_active & enemy, nonpawn_mask & ~king_mask);
    // King captures
    write_moves<MoveFlags::cap_normal>(moves, at, srcs, king_active & enemy & ~danger, king_mask);
    // Castling
    {
      const Square rook_hside = position.rook_info().hside(stm);
      const Square rook_aside = position.rook_info().aside(stm);
      const Bitboard king_bb = king_sq.to_bitboard();

      const auto do_castle = [&](Square rook_sq, i8 rook_dest, i8 king_dest, MoveFlags mf) {
        if (rook_sq.is_valid()) {
          rose_assert(position.board()[rook_sq].ptype() == PieceType::r && position.board()[rook_sq].color() == stm);
          const Bitboard rook_bb = rook_sq.to_bitboard();
          const Bitboard rook_ray = backrank_ray(rook_sq, Square::from_file_and_rank(rook_dest, king_sq.rank()));
          const Bitboard king_ray = backrank_ray(king_sq, Square::from_file_and_rank(king_dest, king_sq.rank()));
          const Bitboard clear = empty | king_bb | rook_bb;
          if ((~clear & rook_ray).is_empty() && ((~clear | danger) & king_ray).is_empty()) {
            moves.push_back(Move::make(king_sq, rook_sq, mf));
          }
        }
      };

      do_castle(rook_aside, 3, 2, MoveFlags::castle_aside);
      do_castle(rook_hside, 5, 6, MoveFlags::castle_hside);
    }
    // Non-pawn quiets
    write_moves<MoveFlags::normal>(moves, at, srcs, nonpawn_active & empty, nonpawn_mask & ~king_mask);
    // King quiets
    write_moves<MoveFlags::normal>(moves, at, srcs, king_active & empty & ~danger, king_mask);
    // Do pawns
    {
      const Bitboard bb = position.bitboard_for<PieceType::p>(stm);
      const auto pawn_empty = pawns::pawn_destination_empty(stm, empty, valid_destinations);
      const auto pawn_moves = pawns::pawn_moves(stm);

      const Bitboard pawn_normal = bb & pawn_empty.normal_move;
      const Bitboard pawn_double = bb & pawn_empty.double_move;

      // Promotions
      {
        const u8 mask = static_cast<u8>(pawn_normal.raw >> pawn_info.promotable_shift);
        moves.write4(mask, pawn_moves.promotions);
      }
      // Relative rank 3-6
      {
        constexpr int normal_shift = 16;
        const u32 mask = static_cast<u32>(pawn_normal.raw >> normal_shift);
        moves.write(mask, pawn_moves.normal_moves);
      }
      // Relative rank 2
      {
        const u8 normal_mask = static_cast<u8>(pawn_normal.raw >> pawn_info.second_rank_shift);
        const u8 double_mask = static_cast<u8>(pawn_double.raw >> pawn_info.second_rank_shift);
        const u16 mask = (static_cast<u16>(double_mask) << 8) | normal_mask;
        moves.write(mask, pawn_moves.double_moves);
      }
    }
  }

  auto MoveGen::generate_moves_one_checker(MoveList& moves, Square king_sq, PieceMask checkers) -> void {
    generate_moves_no_checkers(moves, king_sq);
  }

  auto MoveGen::generate_moves_two_checkers(MoveList& moves, Square king_sq, PieceMask checkers) -> void {
    generate_moves_no_checkers(moves, king_sq);
  }

  MoveGen::MoveGen(const Position& position) :
      m_position(position) {
  }

  auto MoveGen::generate_moves(MoveList& moves) -> void {
    const Color stm = m_position.stm();
    const Square king_sq = m_position.king_sq(stm);
    const PieceMask checkers = m_position.attack_table(!stm).read(king_sq);
    switch (checkers.popcount()) {
    case 0:
      return generate_moves_no_checkers(moves, king_sq);
    case 1:
      return generate_moves_one_checker(moves, king_sq, checkers);
    default:
      return generate_moves_two_checkers(moves, king_sq, checkers);
    }
  }

}  // namespace rose
