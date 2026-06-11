#pragma once

#include "rose/eval/nnue/kyanite.hpp"

namespace rose::eval::nnue {

  using EmbeddedArch = Kyanite<256>;
  using EmbeddedNetwork = EmbeddedArch::Network;

  alignas(EmbeddedNetwork) extern const char g_embedded_network_raw[];

  inline auto embedded_network() -> const EmbeddedNetwork& {
    return *reinterpret_cast<const EmbeddedNetwork*>(&g_embedded_network_raw[0]);
  }

}  // namespace rose::eval::nnue
