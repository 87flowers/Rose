#pragma

#include <array>

#include "rose/common.h"
#include "rose/square.h"
#include "rose/util/types.h"

namespace rose::hash {

  extern const std::array<std::array<u64, 64>, 16> piece_table;
  extern const std::array<u64, 8> enpassant_table;
  extern const std::array<u64, 16> castle_table;
  extern const u64 move;

  inline auto movePiece(Square from, Square to, Place src_place) -> u64 {
    return piece_table[src_place.raw >> 4][from.raw] ^ piece_table[src_place.raw >> 4][to.raw];
  }
  inline auto movePiece(Square from, Square to, Color color, PieceType ptype) -> u64 {
    const usize color_index = color.toIndex() << 3;
    return piece_table[color_index + ptype.raw][from.raw] ^ piece_table[color_index + ptype.raw][to.raw];
  }
  inline auto promo(Square from, Square to, Color active_color, PieceType dest_ptype) -> u64 {
    const usize color_index = active_color.toIndex() << 3;
    return piece_table[color_index + PieceType::p][from.raw] ^ piece_table[color_index + dest_ptype.raw][to.raw];
  }
  inline auto removePiece(Square sq, Place place) -> u64 { return piece_table[place.raw >> 4][sq.raw]; }
  inline auto removePiece(Square sq, Color color, PieceType ptype) -> u64 {
    const usize color_index = color.toIndex() << 3;
    return piece_table[color_index + ptype.raw][sq.raw];
  }

} // namespace rose::hash
