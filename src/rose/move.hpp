#pragma once

#include "rose/common.hpp"
#include "rose/square.hpp"

#include <array>
#include <expected>
#include <fmt/format.h>
#include <string_view>

namespace rose {

  // forward declaration
  struct Position;

  enum class MoveFlags : u16 {
    normal = 0x0000,
    double_push = 0x1000,
    castle_aside = 0x2000,  // classical: queen-side
    castle_hside = 0x3000,  // classical: king-side
    promo_q = 0x4000,
    promo_n = 0x5000,
    promo_r = 0x6000,
    promo_b = 0x7000,
    cap_normal = 0x8000,
    enpassant = 0x9000,
    cap_promo_q = 0xC000,
    cap_promo_n = 0xD000,
    cap_promo_r = 0xE000,
    cap_promo_b = 0xF000,
  };

  struct Move {
    u16 raw = 0;

    constexpr Move() = default;

    explicit constexpr Move(u16 raw) :
        raw(raw) {
    }

    static constexpr auto none() -> Move {
      return Move {0};
    }

    static constexpr auto make(Square from, Square to, MoveFlags flags) -> Move {
      rose_assert(from.is_valid() && to.is_valid());

      u16 result = 0;
      result |= from.raw;
      result |= static_cast<u16>(to.raw) << 6;
      result |= static_cast<u16>(flags);
      return Move {result};
    }

    constexpr auto is_some() const -> bool {
      return raw != 0;
    }

    constexpr auto is_none() const -> bool {
      return raw == 0;
    }

    constexpr auto from() const -> Square {
      return Square {static_cast<u8>(raw & 0x3F)};
    }

    constexpr auto to() const -> Square {
      return Square {static_cast<u8>((raw >> 6) & 0x3F)};
    }

    constexpr auto is_promo() const -> bool {
      return raw & 0x4000;
    }

    constexpr auto is_capture() const -> bool {
      return raw & 0x8000;
    }

    constexpr auto is_noisy() const -> bool {
      // equivalent to: return capture() || flags() == MoveFlags::promo_q;
      return (raw ^ 0x3000) >= 0x7000;
    }

    constexpr auto is_quiet() const -> bool {
      return is_some() && !is_noisy();
    }

    constexpr auto is_castle() const -> bool {
      return flags() == MoveFlags::castle_aside || flags() == MoveFlags::castle_hside;
    }

    constexpr auto is_enpassant() const -> bool {
      return flags() == MoveFlags::enpassant;
    }

    constexpr auto is_double_push() const -> bool {
      return flags() == MoveFlags::double_push;
    }

    constexpr auto ptype() const -> PieceType {
      rose_assert(is_promo());
      constexpr std::array<PieceType, 4> lut {PieceType::q, PieceType::n, PieceType::r, PieceType::b};
      return lut[(raw & 0x3000) >> 12];
    }

    constexpr auto flags() const -> MoveFlags {
      return static_cast<MoveFlags>(raw & 0xF000);
    }

    static auto parse(std::string_view str, MoveFormat format, const Position& context) -> std::expected<Move, ParseError>;

    constexpr auto to_string(MoveFormat format) const -> std::string {
      const bool classical = format == MoveFormat::classical;
      if (is_none())
        return fmt::format("(none)", from(), to());
      if (classical && flags() == MoveFlags::castle_aside && from().file() == 4 && to().file() == 0)
        return fmt::format("{}c{}", from(), static_cast<char>(to().rank() + '1'));
      if (classical && flags() == MoveFlags::castle_hside && from().file() == 4 && to().file() == 7)
        return fmt::format("{}g{}", from(), static_cast<char>(to().rank() + '1'));
      if (is_promo())
        return fmt::format("{}{}{}", from(), to(), ptype());
      else
        return fmt::format("{}{}", from(), to());
    }

    inline constexpr auto operator==(const Move&) const -> bool = default;
  };

}  // namespace rose
