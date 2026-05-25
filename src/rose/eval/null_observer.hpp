#pragma once

#include "rose/common.hpp"
#include "rose/eval/concepts.hpp"
#include "rose/square.hpp"

namespace rose {
  struct Position;
}  // namespace rose

namespace rose::eval {

  struct NullObserver {
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
  };

  static_assert(concepts::Observer<NullObserver>);

}  // namespace rose::eval
