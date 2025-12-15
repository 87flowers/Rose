#pragma once

#include <cstddef>
#include <cstdint>
#include <fmt/format.h>
#include <lps/lps.hpp>
#include <string_view>
#include <utility>

namespace rose {
  struct Bitboard;
}  // namespace rose

namespace rose {

  using namespace lps::prelude;

  using u8 = std::uint8_t;
  using u16 = std::uint16_t;
  using u32 = std::uint32_t;
  using u64 = std::uint64_t;

  using i8 = std::int8_t;
  using i16 = std::int16_t;
  using i32 = std::int32_t;
  using i64 = std::int64_t;

  using isize = std::intptr_t;
  using usize = std::size_t;
  static_assert(sizeof(isize) == sizeof(usize));

  using f32 = float;
  using f64 = double;

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
    enum Underlying : bool {
      white = 0,
      black = 1,
    };

    Underlying raw;

    constexpr Color() = default;

    /* implicit */ constexpr Color(Underlying raw) :
        raw(raw) {
    }

    constexpr auto invert() const -> Color {
      return static_cast<Underlying>(!raw);
    }

    constexpr auto to_back_rank() const -> i8 {
      return raw == white ? 0 : 7;
    }

    constexpr auto to_index() const -> usize {
      return static_cast<usize>(raw);
    }

    constexpr auto to_msb8() const -> u8 {
      return raw == white ? 0 : 0x80;
    }

    constexpr auto to_bitboard() const -> Bitboard;

    constexpr auto to_char() const -> char {
      return "wb"[to_index()];
    }

    constexpr auto operator!() const -> Color {
      return invert();
    }

    constexpr auto operator==(const Color&) const -> bool = default;
  };

  struct PieceType {
    enum Underlying : u8 {
      none = 0b000,
      k = 0b001,
      p = 0b010,
      n = 0b011,
      b = 0b101,
      r = 0b110,
      q = 0b111,
    };

    Underlying raw = none;

    constexpr PieceType() = default;

    /* implicit */ constexpr PieceType(Underlying raw) :
        raw(raw) {
    }

    static constexpr auto from_index(usize index) -> PieceType {
      return static_cast<Underlying>(index);
    }

    constexpr auto is_none() const -> bool {
      return raw == none;
    }

    constexpr auto to_index() const -> usize {
      return std::to_underlying(raw);
    }

    constexpr auto to_sort_value() const -> i32 {
      return (std::to_underlying(raw) - k - 1) & 0b111;
    }

    constexpr auto to_char() const -> char {
      constexpr std::string_view str = ".kpn?brq";
      return str[to_index()];
    }

    constexpr auto is_slider() const -> bool {
      return raw & 0b100;
    }

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

    constexpr auto operator==(const PieceType&) const -> bool = default;
  };

  static_assert(sizeof(PieceType) == sizeof(u8));

}  // namespace rose

template<>
struct fmt::formatter<rose::ParseError, char> {
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
    return ctx.begin();
  }

  template<class FmtContext>
  auto format(rose::ParseError err, FmtContext& ctx) const -> FmtContext::iterator {
    switch (err) {
    case rose::ParseError::invalid_char:
      return fmt::format_to(ctx.out(), "Invalid Character");
    case rose::ParseError::invalid_length:
      return fmt::format_to(ctx.out(), "Invalid Length of Field");
    case rose::ParseError::color_violation:
      return fmt::format_to(ctx.out(), "Color Violation");
    case rose::ParseError::out_of_range:
      return fmt::format_to(ctx.out(), "Value Out Of Range");
    case rose::ParseError::invalid_board:
      return fmt::format_to(ctx.out(), "Invalid Board");
    case rose::ParseError::too_many_kings:
      return fmt::format_to(ctx.out(), "Too Many Kings");
    case rose::ParseError::too_many_pieces:
      return fmt::format_to(ctx.out(), "Too Many Pieces");
    }
    std::unreachable();
  }
};

template<>
struct fmt::formatter<rose::Color, char> {
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
    return ctx.begin();
  }

  template<class FmtContext>
  auto format(rose::Color c, FmtContext& ctx) const -> FmtContext::iterator {
    return fmt::format_to(ctx.out(), "{}", c.to_char());
  }
};

template<>
struct fmt::formatter<rose::PieceType, char> {
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
    return ctx.begin();
  }

  template<class FmtContext>
  auto format(rose::PieceType ptype, FmtContext& ctx) const -> FmtContext::iterator {
    return fmt::format_to(ctx.out(), "{}", ptype.to_char());
  }
};
