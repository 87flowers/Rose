#pragma once

#include "rose/common.hpp"

#include <array>

namespace rose::nnue {

  constexpr usize input_size = 768;
  constexpr usize hl_size = 128;
  constexpr i32 scale = 400;
  constexpr i32 qa = 255;
  constexpr i32 qb = 64;

  using Accumulator = std::array<i16, hl_size>;

  struct Network {
    std::array<Accumulator, input_size> accumulator_weights;
    Accumulator accumulator_biases;
    std::array<Accumulator, 2> output_weights;
    i16 output_bias;
  };

  alignas(Network) extern const char g_embedded_network_raw[];

  inline auto default_network() -> const Network& {
    return *reinterpret_cast<const Network*>(&g_embedded_network_raw[0]);
  }

}  // namespace rose::nnue
