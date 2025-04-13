#include "rose/move.h"

#include <cmath>
#include <expected>
#include <string_view>

#include "rose/common.h"
#include "rose/position.h"
#include "rose/square.h"

namespace rose {

  auto Move::parse(std::string_view str, const Position &context) -> std::expected<Move, ParseError> {
    if (str.size() != 4 && str.size() != 5)
      return std::unexpected(ParseError::invalid_length);

    const auto from = Square::parse(str.substr(0, 2));
    if (!from)
      return std::unexpected(from.error());

    const auto to = Square::parse(str.substr(2, 2));
    if (!to)
      return std::unexpected(to.error());

    const PieceType ptype = context.board().m[from.value().raw].ptype();
    const bool capture = !context.board().m[to.value().raw].isEmpty();

    if (str.size() == 4) {
      if (ptype == PieceType::p) {
        if (context.enpassant() == to.value())
          return make(from.value(), to.value(), MoveFlags::enpassant);
        if (std::abs(from.value().raw - to.value().raw) == 16)
          return make(from.value(), to.value(), MoveFlags::double_push);
      }
      if (ptype == PieceType::k) {
        // TODO: FRC
        if (from.value().file() == 4 && to.value().file() == 2)
          return make(from.value(), context.rookInfo(context.activeColor()).aside, MoveFlags::castle_aside);
        if (from.value().file() == 4 && to.value().file() == 6)
          return make(from.value(), context.rookInfo(context.activeColor()).hside, MoveFlags::castle_hside);
      }
      return make(from.value(), to.value(), capture ? MoveFlags::capture : MoveFlags::normal);
    }

    const auto p = Place::parse(str[4]);
    if (!p || p.value().ptype() == PieceType::k || p.value().ptype() == PieceType::p || p.value().ptype() == PieceType::none)
      return std::unexpected(ParseError::invalid_char);

    return make(from.value(), to.value(), [&] {
      switch (p.value().ptype().raw) {
      case PieceType::q:
        return capture ? MoveFlags::cap_promo_q : MoveFlags::promo_q;
      case PieceType::n:
        return capture ? MoveFlags::cap_promo_n : MoveFlags::promo_n;
      case PieceType::r:
        return capture ? MoveFlags::cap_promo_r : MoveFlags::promo_r;
      case PieceType::b:
        return capture ? MoveFlags::cap_promo_b : MoveFlags::promo_b;
      }
      std::unreachable();
    }());
  }

} // namespace rose
