#include "rose/move.hpp"

#include "rose/common.hpp"
#include "rose/config.hpp"
#include "rose/position.hpp"
#include "rose/square.hpp"

#include <cmath>
#include <expected>
#include <string_view>

namespace rose {

  auto Move::parse(std::string_view str, const Position& context) -> std::expected<Move, ParseError> {
    if (str.size() != 4 && str.size() != 5)
      return std::unexpected(ParseError::invalid_length);

    const auto from = Square::parse(str.substr(0, 2));
    if (!from)
      return std::unexpected(from.error());

    const auto to = Square::parse(str.substr(2, 2));
    if (!to)
      return std::unexpected(to.error());

    const Place src_place = context.board()[*from];
    const Place dest_place = context.board()[*to];

    const PieceType ptype = src_place.ptype();
    const bool capture = !dest_place.is_empty();

    if (src_place.color() != context.stm())
      return std::unexpected(ParseError::color_violation);

    if (str.size() == 4) {
      if (ptype == PieceType::p) {
        if (context.enpassant() == *to)
          return make(*from, *to, MoveFlags::enpassant);
        if (std::abs(from->raw - to->raw) == 16)
          return make(*from, *to, MoveFlags::double_push);
      }
      if (ptype == PieceType::k) {
        if (*to == context.rook_info().aside(context.stm()))
          return make(*from, *to, MoveFlags::castle_aside);
        if (*to == context.rook_info().hside(context.stm()))
          return make(*from, *to, MoveFlags::castle_hside);
        if (!config::frc && from->file() == 4 && to->file() == 2)
          return make(*from, context.rook_info().aside(context.stm()), MoveFlags::castle_aside);
        if (!config::frc && from->file() == 4 && to->file() == 6)
          return make(*from, context.rook_info().hside(context.stm()), MoveFlags::castle_hside);
      }
      return make(*from, *to, capture ? MoveFlags::cap_normal : MoveFlags::normal);
    }

    // This check needs to be here because castling = king captures rook.
    if (capture && dest_place.color() == context.stm())
      return std::unexpected(ParseError::color_violation);

    const auto mf = [&]() -> std::expected<MoveFlags, ParseError> {
      switch (str[4]) {
      case 'q':
        return capture ? MoveFlags::cap_promo_q : MoveFlags::promo_q;
      case 'n':
        return capture ? MoveFlags::cap_promo_n : MoveFlags::promo_n;
      case 'r':
        return capture ? MoveFlags::cap_promo_r : MoveFlags::promo_r;
      case 'b':
        return capture ? MoveFlags::cap_promo_b : MoveFlags::promo_b;
      default:
        return std::unexpected(ParseError::invalid_char);
      }
    }();
    if (!mf)
      return std::unexpected(mf.error());
    return make(*from, *to, mf.value());
  }

}  // namespace rose
