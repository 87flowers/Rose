#pragma once

#include "rose/common.hpp"
#include "rose/nnue/network.hpp"
#include "rose/square.hpp"

#include <algorithm>
#include <array>

namespace rose {
  struct Position;
}  // namespace rose

namespace rose::nnue {

  struct Accumulators {
    std::array<Accumulator, 2> values;

    const Accumulator& get(Color color) const {
      return values[color.to_index()];
    }
  };

  inline auto feature_index(Color perspective, Square sq, PieceType ptype, Color side) -> usize {
    //                          qrb-npk-
    constexpr u32 ptype_lut = 0x43201050;

    usize side_index = side.to_index();
    usize ptype_index = (ptype_lut >> (4 * ptype.to_index())) & 0xf;
    usize square_index = sq.raw;

    if (perspective == Color::black) {
      side_index ^= 1;
      square_index ^= 0b111000;
    }

    return side_index * 64 * 6 + ptype_index * 64 + square_index;
  }

  inline auto add(const Network& net, Accumulator& accumulator, usize feature) -> void {
    for (usize i = 0; i < hl_size; i++)
      accumulator[i] += net.accumulator_weights[feature][i];
  }

  inline auto sub(const Network& net, Accumulator& accumulator, usize feature) -> void {
    for (usize i = 0; i < hl_size; i++)
      accumulator[i] -= net.accumulator_weights[feature][i];
  }

  inline auto screlu(i16 x) -> i32 {
    i32 y = std::clamp<i32>(x, 0, qa);
    return y * y;
  }

  inline auto evaluate(const Accumulator& us, const Accumulator& them) -> i32 {
    const Network& net = default_network();
    i32 output = 0;
    for (usize i = 0; i < hl_size; i++) {
      output += screlu(us[i]) * net.output_weights[0][i];
      output += screlu(them[i]) * net.output_weights[1][i];
    }
    output /= qa;
    output += net.output_bias;
    output *= scale;
    output /= qa * qb;
    return output;
  }

  inline auto evaluate(const Accumulators& accumulators, Color stm) -> i32 {
    return evaluate(accumulators.get(stm), accumulators.get(stm.invert()));
  }

  auto accumulators_from_position(const Position& pos) -> Accumulators;

  auto evaluate(const Position& pos) -> i32;

}  // namespace rose::nnue
