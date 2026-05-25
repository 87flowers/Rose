#include "rose/eval/nnue/embedded.hpp"

namespace rose::eval::nnue {

  alignas(EmbeddedNetwork) const char g_embedded_network_raw[] = {
#embed ROSE_NETWORK_FILE
  };

}  // namespace rose::eval::nnue
