#pragma once

#include "rose/eval/nnue/rose_arch_1.hpp"

namespace rose::eval::nnue {

  namespace embedded_arch = rose_arch_1;
  using EmbeddedNetwork = embedded_arch::Network;

  alignas(EmbeddedNetwork) extern const char g_embedded_network_raw[];

  inline auto embedded_network() -> const EmbeddedNetwork& {
    return *reinterpret_cast<const EmbeddedNetwork*>(&g_embedded_network_raw[0]);
  }

}  // namespace rose::eval::nnue
