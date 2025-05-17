#pragma once

#include <array>
#include <bit>
#include <format>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>

#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose {

  static inline constexpr usize max_legal_moves = 256;
  static inline constexpr i32 max_search_ply = 255;

  enum class ParseError {
    invalid_char,
    invalid_length,
    color_violation,
    out_of_range,
    invalid_board,
    too_many_kings,
    too_many_pieces,
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
      none = 0b000,
      k = 0b001,
      p = 0b010,
      n = 0b011,
      b = 0b101,
      r = 0b110,
      q = 0b111,
    };

    Inner raw = none;

    constexpr PieceType() = default;
    /* implicit */ constexpr PieceType(Inner raw) : raw(raw) {}

    static constexpr auto fromIndex(usize index) -> PieceType { return static_cast<Inner>(index); }

    constexpr auto isNone() const -> bool { return raw == none; }
    constexpr auto toIndex() const -> usize { return std::to_underlying(raw); }
    constexpr auto toSortValue() const -> i32 { return (std::to_underlying(raw) - k - 1) & 0b111; }
    constexpr auto toChar() const -> char {
      constexpr std::string_view str = ".kpn?brq";
      return str[toIndex()];
    }

    constexpr auto isSlider() const -> bool { return raw & 0b100; }

    static constexpr auto parse(char ch) -> std::optional<std::tuple<PieceType, Color>> {
      switch (ch) {
      case 'P':
        return std::tuple(PieceType::p, Color::white);
      case 'N':
        return std::tuple(PieceType::n, Color::white);
      case 'B':
        return std::tuple(PieceType::b, Color::white);
      case 'R':
        return std::tuple(PieceType::r, Color::white);
      case 'Q':
        return std::tuple(PieceType::q, Color::white);
      case 'K':
        return std::tuple(PieceType::k, Color::white);
      case 'p':
        return std::tuple(PieceType::p, Color::black);
      case 'n':
        return std::tuple(PieceType::n, Color::black);
      case 'b':
        return std::tuple(PieceType::b, Color::black);
      case 'r':
        return std::tuple(PieceType::r, Color::black);
      case 'q':
        return std::tuple(PieceType::q, Color::black);
      case 'k':
        return std::tuple(PieceType::k, Color::black);
      }
      return std::nullopt;
    }

    constexpr auto operator==(const PieceType &) const -> bool = default;
  };
  static_assert(sizeof(PieceType) == sizeof(u8));

  struct Place {
    enum Inner : u8 {
      empty = 0,

      color_mask = 0x80,

      white = 0x00,
      black = 0x80,
    };
    inline static constexpr u8 slider_bit = 0b100 << 4;

    Inner raw = empty;

    constexpr Place() = default;
    /* implicit */ constexpr Place(Inner raw) : raw(raw) {}

    static constexpr auto fromColorAndPtypeAndId(Color color, PieceType pt, u8 id) -> Place {
      rose_assert(id < 0x10);
      return static_cast<Inner>(color.toMsb8() | (pt.raw << 4) | id);
    }

    constexpr auto isEmpty() const -> bool { return raw == empty; }
    constexpr auto color() const -> Color { return static_cast<Color::Inner>((raw & black) != 0); }
    constexpr auto ptype() const -> PieceType { return static_cast<PieceType::Inner>((raw >> 4) & 0x7); }
    constexpr auto id() const -> u8 { return raw & 0xF; }

    constexpr auto toColorIndex() const -> usize { return color().toIndex(); }
    constexpr auto toPtypeIndex() const -> usize { return ptype().toIndex(); }
    constexpr auto toChar() const -> char {
      constexpr std::array<std::string_view, 2> str{{".KPN?BRQ", ".kpn?brq"}};
      return str[toColorIndex()][toPtypeIndex()];
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
    case rose::ParseError::color_violation:
      return std::format_to(ctx.out(), "color_violation");
    case rose::ParseError::out_of_range:
      return std::format_to(ctx.out(), "out_of_range");
    case rose::ParseError::invalid_board:
      return std::format_to(ctx.out(), "invalid_board");
    case rose::ParseError::too_many_kings:
      return std::format_to(ctx.out(), "too_many_kings");
    case rose::ParseError::too_many_pieces:
      return std::format_to(ctx.out(), "too_many_pieces");
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

template <> struct std::formatter<rose::PieceType, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(rose::PieceType ptype, FmtContext &ctx) const -> FmtContext::iterator {
    return std::format_to(ctx.out(), "{}", ptype.toChar());
  }
};

template <> struct std::formatter<rose::Place, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(rose::Place place, FmtContext &ctx) const -> FmtContext::iterator {
    return std::format_to(ctx.out(), "{}", place.toChar());
  }
};
