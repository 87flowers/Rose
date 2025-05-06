#pragma once

#include <format>

#include "rose/common.h"
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

template <> struct std::formatter<rose::Line, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(const rose::Line &line, FmtContext &ctx) const -> FmtContext::iterator {
    auto iter = line.pv.begin();
    if (iter != line.pv.end()) {
      ctx.advance_to(std::format_to(ctx.out(), "{}", *iter));
      ++iter;
      for (; iter != line.pv.end(); ++iter) {
        ctx.advance_to(std::format_to(ctx.out(), " {}", *iter));
      }
    }
    return ctx.out();
  }
};
