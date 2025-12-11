#pragma once

#include "rose/common.hpp"
#include "rose/util/assert.hpp"

#include <compare>
#include <expected>
#include <fmt/format.h>
#include <string_view>
#include <tuple>

namespace rose {

  struct Square {
    u8 raw = 0x80;

    static constexpr auto invalid() -> Square {
      return { 0x80 };
    }

    constexpr auto is_valid() const -> bool {
      return (raw & 0x80) == 0;
    }

    constexpr auto file() const -> i8 {
      return raw % 8;
    }

    constexpr auto rank() const -> i8 {
      return raw / 8;
    }

    constexpr auto flip_ranks() const -> Square {
      return { narrow_cast<u8>(raw ^ 0b111000) };
    }

    static constexpr auto from_file_and_rank(i8 file, i8 rank) -> Square {
      return { narrow_cast<u8>(rank * 8 + file) };
    }

    constexpr auto to_file_and_rank() const -> std::tuple<i8, i8> {
      return { raw % 8, raw / 8 };
    }

    constexpr auto to_bitboard() const -> Bitboard;

    static constexpr auto parse(std::string_view str) -> std::expected<Square, ParseError> {
      if (str.size() != 2)
        return std::unexpected(ParseError::invalid_length);
      if (str[0] < 'a' or str[0] > 'h')
        return std::unexpected(ParseError::invalid_char);
      const u8 file = str[0] - 'a';
      if (str[1] < '1' or str[1] > '8')
        return std::unexpected(ParseError::invalid_char);
      const u8 rank = str[1] - '1';
      return from_file_and_rank(file, rank);
    }

    constexpr auto operator<=>(const Square&) const -> std::strong_ordering = default;
  };

}  // namespace rose

template<>
struct fmt::formatter<rose::Square, char> {
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
    return ctx.begin();
  }

  template<class FmtContext>
  auto format(rose::Square sq, FmtContext& ctx) const -> FmtContext::iterator {
    const auto [file, rank] = sq.to_file_and_rank();
    return fmt::format_to(ctx.out(), "{}{}", static_cast<char>(file + 'a'), static_cast<char>(rank + '1'));
  }
};
