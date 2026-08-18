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
  extern const std::array<Hash, 16> castle_table;
  extern const std::array<Hash, 8> enpassant_table;

  inline constexpr Hash move = ~Hash {0};

}  // namespace rose::hash

namespace rose {
  struct Hashes {
    Hash full = 0;
    Hash pawn = 0;
    std::array<Hash, Color::count> non_pawn {};

    constexpr auto operator==(const Hashes&) const -> bool = default;

    inline auto toggle_enpassant(Square enpassant) -> void {
      full ^= hash::enpassant_table[enpassant.file()];
    }

    inline auto toggle_castle(usize index) -> void {
      full ^= hash::castle_table[index];
    }

    inline auto toggle_piece(Square sq, Place place) -> void {
      toggle_piece(place.color(), place.ptype(), hash::piece_table[place.raw >> 4][sq.raw]);
    }

    inline auto toggle_piece(Square sq, Color color, PieceType ptype) -> void {
      const usize color_index = color.to_index() << 3;
      toggle_piece(color, ptype, hash::piece_table[color_index + ptype.raw][sq.raw]);
    }

    inline auto toggle_stm() -> void {
      full ^= hash::move;
    }

  private:
    inline auto toggle_piece(Color color, PieceType ptype, Hash h) -> void {
      full ^= h;

      if (ptype == PieceType::p) {
        pawn ^= h;
      } else {
        non_pawn[color.to_index()] ^= h;
      }
    }
  };
}  // namespace rose
