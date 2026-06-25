#pragma once

#include "rose/common.hpp"
#include "rose/eval/concepts.hpp"
#include "rose/square.hpp"

namespace rose {
  struct Position;
}  // namespace rose

namespace rose::eval {

  struct NullObserver {
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

    auto on_add_focus_threats(const Position& pos, PieceType ptype, Square sq, u64 attacker_mask, u8x64 ray_coords, u8x64 ray_places) -> void {
      rose_unused(pos, ptype, sq, attacker_mask, ray_coords, ray_places);
    }

    auto on_remove_focus_threats(const Position& pos, PieceType ptype, Square sq) -> void {
      rose_unused(pos, ptype, sq);
    }

    auto on_add_discovered_threats(const Position& pos, m8x64 sliders, m8x64 raymask, u8x64 ray_coords, u8x64 ray_places) -> void {
      rose_unused(pos, sliders, raymask, ray_coords, ray_places);
    }

    auto on_remove_discovered_threats(const Position& pos, m8x64 sliders, m8x64 raymask, u8x64 ray_coords, u8x64 ray_places) -> void {
      rose_unused(pos, sliders, raymask, ray_coords, ray_places);
    }

    auto on_finalize(const Position& pos) -> void {
      rose_unused(pos);
    }
  };

  static_assert(concepts::Observer<NullObserver>);

}  // namespace rose::eval
