#include "rose/nnue/network.hpp"

namespace rose::nnue {

  alignas(Network) const char g_embedded_network_raw[] = {
#embed ROSE_NETWORK_FILE
  };

}  // namespace rose::nnue
