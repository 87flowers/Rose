#pragma once

#include "rose/bitboard.hpp"
#include "rose/common.hpp"
#include "rose/square.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/lsb_iterator.hpp"

#include <array>
#include <bit>

namespace rose {

  struct PieceId;
  struct PieceMask;
  struct Place;

  struct PieceId {
    u8 raw = 0;

    constexpr PieceId() = default;

    constexpr PieceId(u8 raw) :
        raw(raw) {
      rose_assert(raw < 0x10);
    }

    static constexpr auto king() -> PieceId {
      return {0};
    }

    constexpr auto to_piece_mask() const -> PieceMask;
  };

  struct PieceMask {
    u16 raw = 0;

    using Iterator = LsbIterator<PieceMask>;

    constexpr PieceMask() = default;

    constexpr explicit PieceMask(u16 raw) :
        raw(raw) {
    }

    static constexpr auto king() -> PieceMask {
      return PieceMask {0x0001};
    }

    constexpr auto is_empty() const -> bool {
      return raw == 0;
    }

    constexpr auto popcount() const -> int {
      return std::popcount(raw);
    }

    constexpr auto lsb() const -> PieceId {
      return PieceId {static_cast<u8>(std::countr_zero(raw))};
    }

    constexpr auto pop_lsb() -> void {
      raw &= raw - 1;
    }

    constexpr auto is_set(PieceId id) const -> bool {
      return (raw >> id.raw) & 1;
    }

    constexpr auto clear(PieceId id) -> void {
      raw &= ~id.to_piece_mask().raw;
    }

    constexpr auto set(PieceId id) -> void {
      raw |= id.to_piece_mask().raw;
    }

    constexpr auto set(PieceId id, bool value) -> void {
      if (value) {
        set(id);
      } else {
        clear(id);
      }
    }

    auto begin() const -> Iterator {
      return Iterator {*this};
    }

    auto end() const -> Iterator {
      return Iterator {PieceMask {}};
    }

    friend constexpr auto operator~(PieceMask a) -> PieceMask {
      return PieceMask {static_cast<u16>(~a.raw)};
    }

    friend constexpr auto operator&(PieceMask a, PieceMask b) -> PieceMask {
      return PieceMask {static_cast<u16>(a.raw & b.raw)};
    }

    friend constexpr auto operator|(PieceMask a, PieceMask b) -> PieceMask {
      return PieceMask {static_cast<u16>(a.raw | b.raw)};
    }

    friend constexpr auto operator&=(PieceMask& a, PieceMask b) -> PieceMask& {
      return a = a & b;
    }

    friend constexpr auto operator|=(PieceMask& a, PieceMask b) -> PieceMask& {
      return a = a | b;
    }

    constexpr auto operator==(const PieceMask&) const -> bool = default;
  };

  constexpr auto PieceId::to_piece_mask() const -> PieceMask {
    return PieceMask {narrow_cast<u16>(1 << raw)};
  }

  static_assert(sizeof(PieceMask) == sizeof(u16));

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

    static constexpr auto make(Color color, PieceType pt, PieceId id) -> Place {
      return static_cast<Underlying>(color.to_msb8() | (pt.raw << 4) | id.raw);
    }

    constexpr auto is_empty() const -> bool {
      return raw == empty;
    }

    constexpr auto color() const -> Color {
      return static_cast<Color::Underlying>((raw & black) != 0);
    }

    constexpr auto ptype() const -> PieceType {
      return static_cast<PieceType::Underlying>((raw >> 4) & 0x7);
    }

    constexpr auto id() const -> PieceId {
      return PieceId {narrow_cast<u8>(raw & 0xF)};
    }

    constexpr auto to_color_index() const -> usize {
      return color().to_index();
    }

    constexpr auto to_ptype_index() const -> usize {
      return ptype().to_index();
    }

    constexpr auto to_char() const -> char {
      constexpr std::array<std::string_view, 2> str {{".KPN?BRQ", ".kpn?brq"}};
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

    auto empty_bitboard() const -> Bitboard {
      return Bitboard {to_vector().zeros().to_bits()};
    }

    auto occupied_bitboard() const -> Bitboard {
      return Bitboard {to_vector().nonzeros().to_bits()};
    }

    auto color_bitboard(Color color) const -> Bitboard {
      return (Bitboard {~to_vector().msb().to_bits()} ^ color.to_bitboard()) & occupied_bitboard();
    }

    template<PieceType ptype>
    auto bitboard_for(Color color) const -> Bitboard {
      const u8 expected_place = Place::make(color, ptype, PieceId {0}).raw;
      return Bitboard {(to_vector() & u8x64::splat(0xF0)).eq(u8x64::splat(expected_place)).to_bits()};
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

  struct alignas(64) Wordboard {
    u16x64 raw;

    constexpr Wordboard() = default;

    constexpr Wordboard(u16x64 value) :
        raw(value) {
    }

    auto to_mailbox() const -> std::array<PieceMask, 64> {
      return std::bit_cast<std::array<PieceMask, 64>>(raw);
    }

    auto bitboard_any() const -> Bitboard {
      return Bitboard {raw.nonzeros().to_bits()};
    }

    auto bitboard_for(PieceMask pm) const -> Bitboard {
      return Bitboard {raw.test(u16x64::splat(pm.raw)).to_bits()};
    }

    auto read(Square sq) const -> PieceMask {
      PieceMask value;
      std::memcpy(&value, reinterpret_cast<const char*>(&raw) + sq.raw * sizeof(PieceMask), sizeof(PieceMask));
      return value;
    }

    constexpr auto operator==(const Wordboard& other) const -> bool = default;
  };

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
