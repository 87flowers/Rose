#pragma once

#include <atomic>

#include "rose/util/types.h"

namespace rose {
  struct SearchStats {
    std::atomic<u64> nodes{0};
    void reset() { nodes.store(0); }
  };
} // namespace rose
