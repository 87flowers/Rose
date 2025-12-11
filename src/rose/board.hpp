#pragma once

#include "rose/common.hpp"
#include "rose/square.hpp"
#include "rose/util/assert.hpp"

#include <array>
#include <bit>

namespace rose {

  struct PieceId {
    u8 raw;

    constexpr PieceId(u8 raw) :
        raw(raw) {
      rose_assert(raw < 0x10);
    }

    static constexpr auto king() -> PieceId {
      return { 0 };
    }

    constexpr auto to_piece_mask() const -> u16 {
      return narrow_cast<u16>(1 << raw);
    }
  };

  struct Place {
    enum Underlying : u8 {
      empty = 0,

      color_mask = 0x80,

      white = 0x00,
      black = 0x80,
    };

    inline static constexpr u8 slider_bit = 0b100 << 4;

    Underlying raw = empty;

    constexpr Place() = default;

    /* implicit */ constexpr Place(Underlying raw) :
        raw(raw) {
    }

    static constexpr auto from(Color color, PieceType pt, PieceId id) -> Place {
      return static_cast<Underlying>(color.to_msb8() | (pt.raw << 4) | id.raw);
    }

    constexpr auto isEmpty() const -> bool {
      return raw == empty;
    }

    constexpr auto color() const -> Color {
      return static_cast<Color::Underlying>((raw & black) != 0);
    }

    constexpr auto ptype() const -> PieceType {
      return static_cast<PieceType::Underlying>((raw >> 4) & 0x7);
    }

    constexpr auto id() const -> PieceId {
      return PieceId { narrow_cast<u8>(raw & 0xF) };
    }

    constexpr auto to_color_index() const -> usize {
      return color().to_index();
    }

    constexpr auto to_ptype_index() const -> usize {
      return ptype().to_index();
    }

    constexpr auto to_char() const -> char {
      constexpr std::array<std::string_view, 2> str {
        { ".KPN?BRQ", ".kpn?brq" }
      };
      return str[to_color_index()][to_ptype_index()];
    }

    inline constexpr auto operator==(const Place&) const -> bool = default;
  };

  static_assert(sizeof(Place) == sizeof(u8));

  struct alignas(64) Byteboard {
    std::array<Place, 64> mailbox;

    auto to_vector() const -> u8x64 {
      return std::bit_cast<u8x64>(mailbox);
    }

    constexpr auto operator[](Square sq) -> Place& {
      return mailbox[sq.raw];
    }

    constexpr auto operator[](Square sq) const -> Place {
      return mailbox[sq.raw];
    }

    bool operator==(const Byteboard& other) const = default;
  };

  static_assert(sizeof(Byteboard) == 64);

}  // namespace rose

template<>
struct fmt::formatter<rose::Place, char> {
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
    return ctx.begin();
  }

  template<class FmtContext>
  auto format(rose::Place place, FmtContext& ctx) const -> FmtContext::iterator {
    return fmt::format_to(ctx.out(), "{}", place.to_char());
  }
};
