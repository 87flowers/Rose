#pragma once

#include <compare>
#include <expected>
#include <string_view>
#include <tuple>

#include "rose/common.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose {

  struct Square {
    u8 raw = 0x80;

    static constexpr auto invalid() -> Square { return {0x80}; }

    constexpr auto isValid() const -> bool { return (raw & 0x80) == 0; }
    constexpr auto file() const -> int { return raw % 8; }
    constexpr auto rank() const -> int { return raw / 8; }

    static constexpr auto fromFileAndRank(usize file, usize rank) -> Square { return Square{narrow_cast<u8>(rank * 8 + file)}; }
    constexpr auto toFileAndRank() const -> std::tuple<usize, usize> { return {raw % 8, raw / 8}; }

    static constexpr auto parse(std::string_view str) -> std::expected<Square, ParseError> {
      if (str.size() != 2)
        return std::unexpected(ParseError::invalid_length);
      if (str[0] < 'a' or str[0] > 'h')
        return std::unexpected(ParseError::invalid_char);
      const u8 file = str[0] - 'a';
      if (str[1] < '1' or str[1] > '8')
        return std::unexpected(ParseError::invalid_char);
      const u8 rank = str[1] - '1';
      return fromFileAndRank(file, rank);
    }

    constexpr auto operator<=>(const Square &) const -> std::strong_ordering = default;
  };

} // namespace rose

template <> struct std::formatter<rose::Square, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(rose::Square sq, FmtContext &ctx) const -> FmtContext::iterator {
    const auto [file, rank] = sq.toFileAndRank();
    return std::format_to(ctx.out(), "{}{}", static_cast<char>(file + 'a'), static_cast<char>(rank + '1'));
  }
};
