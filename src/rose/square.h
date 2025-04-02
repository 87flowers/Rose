#pragma once

#include <compare>
#include <expected>
#include <string_view>
#include <tuple>
#include <x86intrin.h>

#include "rose/common.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  struct Square {
    u8 raw = 0x80;

    static constexpr auto invalid() -> Square { return {0x80}; }

    constexpr auto isValid() const -> bool { return (raw & 0x80) == 0; }

    static constexpr auto fromFileAndRank(usize file, usize rank) -> Square { return Square{narrow_cast<u8>(rank * 8 + file)}; }
    constexpr auto toFileAndRank() const -> std::tuple<usize, usize> { return {raw % 8, raw / 8}; }

    auto allAttacks() const -> v512 {
      const v512 offsets = v512::fromArray({
          // clang-format off
          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
          0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, 0x79,
          0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97,
          0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99,
          0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
          0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
          0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09,
          0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90,
          0xDF, 0xE1, 0xEE, 0x0E, 0xF2, 0x12, 0x1F, 0x21,
          // clang-format on
      });
      return vec::add8(v512::broadcast8(raw), offsets);
    }

    auto queenRays() const -> v512 {
      const v512 offsets = v512::fromArray({
          // clang-format off
          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
          0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, 0x79,
          0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97,
          0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99,
          0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
          0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
          0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09,
          0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90,
          0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
          // clang-format on
      });
      return vec::add8(v512::broadcast8(raw), offsets);
    }

    auto orthogonalRays() const -> v256 {
      const v256 offsets = v256::fromArray({
          // clang-format off
          0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
          0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
          0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09,
          0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90,
          0x88, 0x88, 0x88, 0x88,
          // clang-format on
      });
      return vec::add8(v256::broadcast8(raw), offsets);
    }

    auto diagonalRays() const -> v256 {
      const v256 offsets = v256::fromArray({
          // clang-format off
          0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
          0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, 0x79,
          0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6, 0x97,
          0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99,
          0x88, 0x88, 0x88, 0x88,
          // clang-format on
      });
      return vec::add8(v256::broadcast8(raw), offsets);
    }

    auto kingLeaps() const -> v128 {
      const v128 offsets = v128::fromArray({
          // clang-format off
          0x01, 0x10, 0x0F, 0xF0, 0x11, 0x1F, 0xF1, 0xFF,
          0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
          // clang-format on
      });
      return vec::add8(v128::broadcast8(raw), offsets);
    }

    auto knightLeaps() const -> v128 {
      const v128 offsets = v128::fromArray({
          // clang-format off
          0xDF, 0xE1, 0xEE, 0x0E, 0xF2, 0x12, 0x1F, 0x21,
          0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
          // clang-format on
      });
      return vec::add8(v128::broadcast8(raw), offsets);
    }

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
