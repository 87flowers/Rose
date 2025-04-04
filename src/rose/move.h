#pragma once

#include "rose/common.h"
#include "rose/square.h"
#include "rose/util/types.h"

namespace rose {

  struct Move {
    u16 raw = 0;

    constexpr Move() = default;
    explicit constexpr Move(u16 raw) : raw(raw) {}

    static constexpr Move none() { return Move{0}; };

    static constexpr auto make(Square from, Square to) -> Move {
      rose_assert(from.isValid() && to.isValid());

      u16 result = 0;
      result |= from.raw;
      result |= static_cast<u16>(to.raw) << 6;
      return Move{result};
    }

    static constexpr auto makePromotion(Square from, Square to, PieceType dest_ptype) -> Move {
      const usize ptype_index = dest_ptype.toIndex();

      rose_assert(from.isValid() && to.isValid());
      rose_assert(ptype_index >= 2 && ptype_index <= 5);

      u16 result = 0;
      result |= from.raw;
      result |= static_cast<u16>(to.raw) << 6;
      result |= static_cast<u16>(ptype_index) << 12;
      return Move{result};
    }

    constexpr auto from() const -> Square { return Square{static_cast<u8>(raw & 0x3F)}; }
    constexpr auto to() const -> Square { return Square{static_cast<u8>((raw >> 6) & 0x3F)}; }
    constexpr auto promo() const -> bool { return raw >> 12; }
    constexpr auto ptype() const -> PieceType { return PieceType::fromIndex(raw >> 12); }

    static constexpr auto parse(std::string_view str) -> std::expected<Move, ParseError> {
      if (str.size() != 4 && str.size() != 5)
        return std::unexpected(ParseError::invalid_length);

      const auto from = Square::parse(str.substr(0, 2));
      if (!from)
        return std::unexpected(from.error());

      const auto to = Square::parse(str.substr(2, 2));
      if (!to)
        return std::unexpected(to.error());

      if (str.size() == 4)
        return Move::make(from.value(), to.value());

      const auto p = Place::parse(str[4]);
      if (!p || p.value().ptype() == PieceType::king || p.value().ptype() == PieceType::pawn || p.value().ptype() == PieceType::none)
        return std::unexpected(ParseError::invalid_char);

      return makePromotion(from.value(), to.value(), p.value().ptype());
    }

    inline constexpr auto operator==(const Move &) const -> bool = default;
  };

} // namespace rose
