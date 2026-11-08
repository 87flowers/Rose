#pragma once

#include "rose/common.hpp"
#include "rose/eval/concepts.hpp"
#include "rose/move.hpp"
#include "rose/square.hpp"

namespace rose {
  struct Position;
}  // namespace rose

namespace rose::eval {

  struct NullObserver {
    auto on_begin(const Position& pos, Move m) -> void {
      rose_unused(pos, m);
    }

    auto on_king_move(const Position& pos, Color stm, Square from, Square to) -> void {
      rose_unused(pos, stm, from, to);
    }

    auto on_add(const Position& pos, Color side, PieceType ptype, Square sq) -> void {
      rose_unused(pos, side, ptype, sq);
    }

    auto on_remove(const Position& pos, Color side, PieceType ptype, Square sq) -> void {
      rose_unused(pos, side, ptype, sq);
    }

    auto on_mutate(const Position& pos, Color side, PieceType src_ptype, PieceType dst_ptype, Square sq) -> void {
      rose_unused(pos, side, src_ptype, dst_ptype, sq);
    }

    auto on_move(const Position& pos, Color side, PieceType ptype, Square from, Square to) -> void {
      rose_unused(pos, side, ptype, from, to);
    }

    auto on_promote(const Position& pos, Color side, PieceType dst_ptype, Square from, Square to) -> void {
      rose_unused(pos, side, dst_ptype, from, to);
    }

    auto on_add_focus_threats(const Position& pos, Color color, PieceType ptype, Square sq, m8x64 add_mask) -> void {
      rose_unused(pos, ptype, sq, add_mask);
    }

    auto on_remove_focus_threats(const Position& pos, Color color, PieceType ptype, Square sq, Bitboard bb) -> void {
      rose_unused(pos, ptype, sq, bb);
    }

    auto on_add_discovered_threats(const Position& pos, Bitboard occ, m8x64 slider_colors, u8x64 slider_id) -> void {
      rose_unused(pos, slider_colors, slider_id);
    }

    auto on_remove_discovered_threats(const Position& pos, Bitboard occ, m8x64 slider_colors, u8x64 slider_id) -> void {
      rose_unused(pos, slider_colors, slider_id);
    }

    auto on_finalize(const Position& pos) -> void {
      rose_unused(pos);
    }
  };

  static_assert(concepts::Observer<NullObserver>);

}  // namespace rose::eval
