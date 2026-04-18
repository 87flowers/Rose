#pragma once

#include "rose/common.hpp"

#include <atomic>

namespace rose {
  struct SearchStats {
    std::atomic<u64> nodes {0};

    void reset() {
      nodes.store(0);
    }
  };
}  // namespace rose
