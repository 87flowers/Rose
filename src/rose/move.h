#pragma once

#include <array>
#include <expected>
#include <string_view>

#include "rose/common.h"
#include "rose/config.h"
#include "rose/square.h"
#include "rose/util/types.h"

namespace rose {

  // forward declaration
  struct Position;

  enum class MoveFlags : u16 {
    normal = 0x0000,
    double_push = 0x1000,
    castle_aside = 0x2000, // classical: queen-side
    castle_hside = 0x3000, // classical: king-side
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
    explicit constexpr Move(u16 raw) : raw(raw) {}

    static constexpr Move none() { return Move{0}; };

    static constexpr auto make(Square from, Square to, MoveFlags flags) -> Move {
      rose_assert(from.isValid() && to.isValid());

      u16 result = 0;
      result |= from.raw;
      result |= static_cast<u16>(to.raw) << 6;
      result |= static_cast<u16>(flags);
      return Move{result};
    }

    constexpr auto from() const -> Square { return Square{static_cast<u8>(raw & 0x3F)}; }
    constexpr auto to() const -> Square { return Square{static_cast<u8>((raw >> 6) & 0x3F)}; }
    constexpr auto promo() const -> bool { return raw & 0x4000; }
    constexpr auto capture() const -> bool { return raw & 0x8000; }
    constexpr auto ptype() const -> PieceType {
      rose_assert(promo());
      constexpr std::array<PieceType, 4> lut{PieceType::q, PieceType::n, PieceType::r, PieceType::b};
      return lut[(raw & 0x3000) >> 12];
    }
    constexpr auto flags() const -> MoveFlags { return static_cast<MoveFlags>(raw & 0xF000); }

    static auto parse(std::string_view str, const Position &context) -> std::expected<Move, ParseError>;

    inline constexpr auto operator==(const Move &) const -> bool = default;
  };

} // namespace rose
