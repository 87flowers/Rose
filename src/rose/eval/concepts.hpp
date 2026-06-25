#pragma once

#include "rose/common.hpp"
#include "rose/square.hpp"

#include <concepts>

namespace rose {
  struct Position;
}  // namespace rose

namespace rose::eval::concepts {

  template<typename T>
  concept Observer = requires(T o,
                              const Position& pos,
                              Color stm,
                              PieceType src_ptype,
                              PieceType dst_ptype,
                              PieceType ptype,
                              Square from,
                              Square to,
                              Square sq,
                              u64 attacker_mask,
                              m8x64 sliders,
                              m8x64 raymask,
                              u8x64 ray_coords,
                              u8x64 ray_places) {
    { o.on_king_move(pos, stm, from, to) } -> std::same_as<void>;
    { o.on_add(pos, stm, ptype, sq) } -> std::same_as<void>;
    { o.on_remove(pos, stm, ptype, sq) } -> std::same_as<void>;
    { o.on_mutate(pos, stm, src_ptype, dst_ptype, sq) } -> std::same_as<void>;
    { o.on_move(pos, stm, ptype, from, to) } -> std::same_as<void>;
    { o.on_promote(pos, stm, dst_ptype, from, to) } -> std::same_as<void>;
    { o.on_add_focus_threats(pos, ptype, sq, attacker_mask, ray_coords, ray_places) } -> std::same_as<void>;
    { o.on_remove_focus_threats(pos, ptype, sq) } -> std::same_as<void>;
    { o.on_add_discovered_threats(pos, sliders, raymask, ray_coords, ray_places) } -> std::same_as<void>;
    { o.on_remove_discovered_threats(pos, sliders, raymask, ray_coords, ray_places) } -> std::same_as<void>;
    { o.on_finalize(pos) } -> std::same_as<void>;
  };

  template<typename T>
  concept State = requires(T s, const Position& pos) {
    { s.reset(pos) } -> std::same_as<void>;
    { s.push() } -> std::same_as<void>;
    { s.pop() } -> std::same_as<void>;
    { s.observer() } -> Observer;
  };

}  // namespace rose::eval::concepts
