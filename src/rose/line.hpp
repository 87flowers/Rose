#pragma once

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/util/static_vector.hpp"

#include <fmt/format.h>

namespace rose {

  struct Line {
    StaticVector<Move, max_search_ply + 1> pv {};

    auto clear() -> void {
      pv.clear();
    }

    auto write(Move m) -> void {
      pv.clear();
      pv.push_back(m);
    }

    auto write(Move m, Line&& child) -> void {
      pv.clear();
      pv.push_back(m);
      pv.append(std::move(child.pv));
    }

    auto to_string(MoveFormat format) const -> std::string {
      std::string output;
      auto iter = pv.begin();
      if (iter != pv.end()) {
        output += iter->to_string(format);
        ++iter;
        for (; iter != pv.end(); ++iter)
          output += iter->to_string(format);
      }
      return output;
    }
  };

}  // namespace rose
