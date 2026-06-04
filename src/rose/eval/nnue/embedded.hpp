#pragma once

#include "rose/eval/nnue/jasper.hpp"

namespace rose::eval::nnue {

  using EmbeddedArch = Jasper<256>;
  using EmbeddedNetwork = EmbeddedArch::Network;

  alignas(EmbeddedNetwork) extern const char g_embedded_network_raw[];

  inline auto embedded_network() -> const EmbeddedNetwork& {
    return *reinterpret_cast<const EmbeddedNetwork*>(&g_embedded_network_raw[0]);
  }

}  // namespace rose::eval::nnue
