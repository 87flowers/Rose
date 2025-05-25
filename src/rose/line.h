#pragma once

#include <format>

#include "rose/common.h"
#include "rose/move.h"
#include "rose/util/static_vector.h"

namespace rose {

  struct Line {
    StaticVector<Move, max_search_ply + 1> pv{};

    auto clear() -> void { pv.clear(); }

    auto write(Move m) -> void {
      pv.clear();
      pv.push_back(m);
    }

    auto write(Move m, Line &&child) -> void {
      pv.clear();
      pv.push_back(m);
      pv.append(std::move(child.pv));
    }
  };

} // namespace rose
