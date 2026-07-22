#pragma once

#include "rose/common.hpp"
#include "rose/util/assert.hpp"

#include <fmt/format.h>
#include <utility>

namespace rose {

  struct NodeType {
    inline static constexpr usize count = 4;

    enum Underlying : u8 {
      none = 0b00,
      cut = 0b01,
      pv = 0b10,
      all = 0b11,
    };

    Underlying raw;

    constexpr NodeType() = default;

    /* implicit */ constexpr NodeType(Underlying raw) :
        raw(raw) {
    }

    constexpr auto to_index() const -> usize {
      return static_cast<usize>(raw);
    }

    constexpr auto is_pv_or_cut() const -> bool {
      return raw == pv || raw == cut;
    }

    constexpr auto is_pv_or_all() const -> bool {
      return raw == pv || raw == all;
    }

    constexpr auto narrow() const -> NodeType {
      rose_assert(raw != none);
      switch (raw) {
      case none:
        return none;
      case cut:
        return cut;
      case pv:
        return cut;
      case all:
        return all;
      }
      std::unreachable();
    }

    constexpr auto next() const -> NodeType {
      rose_assert(raw != none);
      switch (raw) {
      case none:
        return none;
      case cut:
        return all;
      case pv:
        return cut;
      case all:
        return cut;
      }
      std::unreachable();
    }

    constexpr auto operator==(const NodeType&) const -> bool = default;
  };

}  // namespace rose

template<>
struct fmt::formatter<rose::NodeType, char> {
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
    return ctx.begin();
  }

  template<class FmtContext>
  auto format(rose::NodeType nodetype, FmtContext& ctx) const -> FmtContext::iterator {
    switch (nodetype.raw) {
    case rose::NodeType::none:
      return fmt::format_to(ctx.out(), "none");
    case rose::NodeType::cut:
      return fmt::format_to(ctx.out(), "cut node");
    case rose::NodeType::pv:
      return fmt::format_to(ctx.out(), "pv node");
    case rose::NodeType::all:
      return fmt::format_to(ctx.out(), "all node");
    default:
      return fmt::format_to(ctx.out(), "invalid");
    }
    std::unreachable();
  }
};
