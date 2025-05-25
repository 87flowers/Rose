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

  inline constexpr int move_flags_shift = 10;

  enum class MoveFlags : u16 {
    normal = 0x0 << move_flags_shift,
    double_push = 0x1 << move_flags_shift,
    castle_aside = 0x2 << move_flags_shift, // classical: queen-side
    castle_hside = 0x3 << move_flags_shift, // classical: king-side
    promo_q = 0x4 << move_flags_shift,
    promo_n = 0x5 << move_flags_shift,
    promo_r = 0x6 << move_flags_shift,
    promo_b = 0x7 << move_flags_shift,
    cap_normal = 0x8 << move_flags_shift,
    enpassant = 0x9 << move_flags_shift,
    cap_promo_q = 0xC << move_flags_shift,
    cap_promo_n = 0xD << move_flags_shift,
    cap_promo_r = 0xE << move_flags_shift,
    cap_promo_b = 0xF << move_flags_shift,
  };

  struct Move {
    u16 raw = 0;

    constexpr Move() = default;
    explicit constexpr Move(u16 raw) : raw(raw) {}

    static constexpr Move none() { return Move{0}; };

    static constexpr auto make(PieceId id, Square to, MoveFlags flags) -> Move {
      rose_assert(to.isValid());

      u16 result = 0;
      result |= static_cast<u16>(to.raw) << 4;
      result |= id.raw;
      result |= static_cast<u16>(flags);
      return Move{result};
    }

    constexpr auto to() const -> Square { return {static_cast<u8>((raw >> 4) & 0x3F)}; }
    constexpr auto id() const -> PieceId { return {static_cast<u8>(raw & 0xF)}; }
    constexpr auto promo() const -> bool { return raw & (0x4 << move_flags_shift); }
    constexpr auto capture() const -> bool { return raw & (0x8 << move_flags_shift); }
    constexpr auto ptype() const -> PieceType {
      rose_assert(promo());
      constexpr std::array<PieceType, 4> lut{PieceType::q, PieceType::n, PieceType::r, PieceType::b};
      return lut[(raw >> move_flags_shift) & 0x3];
    }
    constexpr auto flags() const -> MoveFlags { return static_cast<MoveFlags>(raw & (0xF << move_flags_shift)); }

    auto from(const Position &context) const -> Square;

    static auto parse(std::string_view str, const Position &context) -> std::expected<Move, ParseError>;

    inline constexpr auto operator==(const Move &) const -> bool = default;
  };

} // namespace rose
