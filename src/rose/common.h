#pragma once

#include <array>
#include <bit>
#include <format>
#include <optional>
#include <string_view>
#include <utility>

#include "rose/util/types.h"

namespace rose {

  static inline constexpr usize max_legal_moves = 256;

  enum class ParseError {
    invalid_char,
    invalid_length,
    out_of_range,
    invalid_board,
    too_many_kings,
  };

  struct Color {
    enum Inner : bool {
      white = 0,
      black = 1,
    };

    Inner raw;

    constexpr Color() = default;
    /* implicit */ constexpr Color(Inner raw) : raw(raw) {}

    constexpr auto invert() const -> Color { return static_cast<Inner>(!raw); }
    constexpr auto toBackRank() const -> u8 { return raw == white ? 0 : 7; }
    constexpr auto toIndex() const -> usize { return static_cast<usize>(raw); }
    constexpr auto toMsb8() const -> u8 { return raw == white ? 0 : 0x80; }
    constexpr auto toBitboard() const -> u64 { return static_cast<u64>(-static_cast<i64>(raw)); }

    constexpr auto operator==(const Color &) const -> bool = default;
  };

  struct PieceType {
    enum Inner : u8 {
      none = 0x00,
      p = 0x01,
      n = 0x02,
      b = 0x04,
      r = 0x08,
      q = 0x1C,
      k = 0x20,
    };

    Inner raw = none;

    constexpr PieceType() = default;
    /* implicit */ constexpr PieceType(Inner raw) : raw(raw) {}

    static constexpr auto fromIndex(usize index) -> PieceType {
      constexpr std::array<PieceType::Inner, 7> ptypes{none, p, n, b, r, q, k};
      return ptypes[index];
    }

    constexpr auto isNone() const -> bool { return raw == none; }
    constexpr auto toIndex() const -> usize { return std::bit_width(std::to_underlying(raw)); }

    constexpr auto operator==(const PieceType &) const -> bool = default;
  };
  static_assert(sizeof(PieceType) == sizeof(u8));

  struct Place {
    enum Inner : u8 {
      empty = 0,

      color_mask = 0x80,

      white = 0x00,
      black = 0x80,

      unmoved = 0x40,

      ptype_mask = 0x3F,

      wp = white | PieceType::p,
      wn = white | PieceType::n,
      wb = white | PieceType::b,
      wr = white | PieceType::r,
      wq = white | PieceType::q,
      wk = white | PieceType::k,

      bp = black | PieceType::p,
      bn = black | PieceType::n,
      bb = black | PieceType::b,
      br = black | PieceType::r,
      bq = black | PieceType::q,
      bk = black | PieceType::k,

      unmoved_wr = unmoved | wr,
      unmoved_wk = unmoved | wk,
      unmoved_br = unmoved | br,
      unmoved_bk = unmoved | bk,
    };

    Inner raw = empty;

    constexpr Place() = default;
    /* implicit */ constexpr Place(Inner raw) : raw(raw) {}

    static constexpr auto fromColorAndPtype(Color color, PieceType pt) -> Place { return static_cast<Inner>(color.toMsb8() | pt.raw); }

    constexpr auto isEmpty() const -> bool { return raw == empty; }
    constexpr auto color() const -> Color { return static_cast<Color::Inner>((raw & black) != 0); }
    constexpr auto ptype() const -> PieceType { return static_cast<PieceType::Inner>(raw & ptype_mask); }
    constexpr auto moved() const -> bool { return !(raw & unmoved); }

    constexpr auto toMoved() const -> Place { return static_cast<Inner>(raw & ~unmoved); }
    constexpr auto toPristine() const -> Place { return static_cast<Inner>(raw | unmoved); }

    constexpr auto toColorIndex() const -> usize { return color().toIndex(); }
    constexpr auto toPtypeIndex() const -> usize { return ptype().toIndex(); }
    constexpr auto toChar() const -> char {
      constexpr std::array<std::string_view, 2> str{{".PNBRQK", ".pnbrqk"}};
      return str[toColorIndex()][toPtypeIndex()];
    }

    static constexpr auto parse(char ch) -> std::optional<Place> {
      switch (ch) {
      case 'P':
        return wp;
      case 'N':
        return wn;
      case 'B':
        return wb;
      case 'R':
        return wr;
      case 'Q':
        return wq;
      case 'K':
        return wk;
      case 'p':
        return bp;
      case 'n':
        return bn;
      case 'b':
        return bb;
      case 'r':
        return br;
      case 'q':
        return bq;
      case 'k':
        return bk;
      }
      return std::nullopt;
    }

    inline constexpr auto operator==(const Place &) const -> bool = default;
  };
  static_assert(sizeof(Place) == sizeof(u8));

}; // namespace rose

template <> struct std::formatter<rose::ParseError, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(rose::ParseError err, FmtContext &ctx) const -> FmtContext::iterator {
    switch (err) {
    case rose::ParseError::invalid_char:
      return std::format_to(ctx.out(), "invalid_char");
    case rose::ParseError::invalid_length:
      return std::format_to(ctx.out(), "invalid_length");
    case rose::ParseError::out_of_range:
      return std::format_to(ctx.out(), "out_of_range");
    case rose::ParseError::invalid_board:
      return std::format_to(ctx.out(), "invalid_board");
    case rose::ParseError::too_many_kings:
      return std::format_to(ctx.out(), "too_many_kings");
    }
    std::unreachable();
  }
};

template <> struct std::formatter<rose::Color, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(rose::Color c, FmtContext &ctx) const -> FmtContext::iterator {
    return std::format_to(ctx.out(), "{}", c == rose::Color::black ? 'b' : 'w');
  }
};

template <> struct std::formatter<rose::Place, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(rose::Place ptype, FmtContext &ctx) const -> FmtContext::iterator {
    return std::format_to(ctx.out(), "{}", ptype.toChar());
  }
};
