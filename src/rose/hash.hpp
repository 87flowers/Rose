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
  private:
    Hash m_full = 0;
    std::array<Hash, Color::count> m_non_pawn {};
    std::array<Hash, PieceType::count> m_piece {};

    inline auto toggle_piece(Color color, PieceType ptype, Hash h) -> void {
      m_full ^= h;
      if (ptype != PieceType::p)
        m_non_pawn[color.to_index()] ^= h;
      m_piece[ptype.to_index()] ^= h;
    }

  public:
    auto full() const -> Hash {
      return m_full;
    }

    auto pawn() const -> Hash {
      return m_piece[PieceType::p];
    }

    auto non_pawn(Color color) const -> Hash {
      return m_non_pawn[color.to_index()];
    }

    auto minor() const -> Hash {
      return m_piece[PieceType::k] ^ m_piece[PieceType::n] ^ m_piece[PieceType::b];
    }

    inline auto toggle_enpassant(Square enpassant) -> void {
      m_full ^= hash::enpassant_table[enpassant.file()];
    }

    inline auto toggle_castle(usize index) -> void {
      m_full ^= hash::castle_table[index];
    }

    inline auto toggle_piece(Square sq, Place place) -> void {
      toggle_piece(place.color(), place.ptype(), hash::piece_table[place.raw >> 4][sq.raw]);
    }

    inline auto toggle_piece(Square sq, Color color, PieceType ptype) -> void {
      const usize color_index = color.to_index() << 3;
      toggle_piece(color, ptype, hash::piece_table[color_index + ptype.raw][sq.raw]);
    }

    inline auto toggle_stm() -> void {
      m_full ^= hash::move;
    }

    constexpr auto operator==(const Hashes&) const -> bool = default;
  };
}  // namespace rose
