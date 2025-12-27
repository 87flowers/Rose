#pragma once

#include "rose/board.hpp"
#include "rose/common.hpp"
#include "rose/square.hpp"

#include <array>

namespace rose {
  using Hash = u64;
}  // namespace rose

namespace rose::hash {

  extern const std::array<std::array<Hash, 64>, 16> piece_table;
  extern const std::array<Hash, 8> castle_table;
  extern const std::array<Hash, 8> enpassant_table;

  inline constexpr Hash move = ~Hash {0};

  inline auto move_piece(Square from, Square to, Place src_place) -> Hash {
    return piece_table[src_place.raw >> 4][from.raw] ^ piece_table[src_place.raw >> 4][to.raw];
  }

  inline auto move_piece(Square from, Square to, Color color, PieceType ptype) -> Hash {
    const usize color_index = color.to_index() << 3;
    return piece_table[color_index + ptype.raw][from.raw] ^ piece_table[color_index + ptype.raw][to.raw];
  }

  inline auto promo(Square from, Square to, Color color, PieceType dest_ptype) -> Hash {
    const usize color_index = color.to_index() << 3;
    return piece_table[color_index + PieceType::p][from.raw] ^ piece_table[color_index + dest_ptype.raw][to.raw];
  }

  inline auto remove_piece(Square sq, Place place) -> Hash {
    return piece_table[place.raw >> 4][sq.raw];
  }

  inline auto remove_piece(Square sq, Color color, PieceType ptype) -> Hash {
    const usize color_index = color.to_index() << 3;
    return piece_table[color_index + ptype.raw][sq.raw];
  }

}  // namespace rose::hash
