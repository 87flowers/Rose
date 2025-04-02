#pragma once

#include <format>
#include <utility>

#include "rose/byte_board.h"
#include "rose/common.h"
#include "rose/square.h"
#include "rose/util/tokenizer.h"
#include "rose/util/types.h"

namespace rose {

  enum class Castling {
    none = 0,
    wq = 0b0001,
    wk = 0b0010,
    bq = 0b0100,
    bk = 0b1000,
  };
  inline constexpr auto operator|(Castling a, Castling b) -> Castling { return static_cast<Castling>(std::to_underlying(a) | std::to_underlying(b)); }
  inline constexpr auto operator&(Castling a, Castling b) -> Castling { return static_cast<Castling>(std::to_underlying(a) & std::to_underlying(b)); }
  inline constexpr auto operator|=(Castling &a, Castling b) -> Castling & { return a = a | b; }
  inline constexpr auto operator&=(Castling &a, Castling b) -> Castling & { return a = a & b; }

  struct Position {
  private:
    friend class std::formatter<rose::Position, char>;

    Byteboard board{};
    Color active_color{};
    u16 ply{};
    u16 irreversible_clock{};
    Square enpassant = Square::invalid();
    u64 hash{};

  public:
    static const Position startpos;

    constexpr Position() = default;

    auto castlingRights() const -> Castling;

    auto prettyPrint() const -> void;

    static auto parse(std::string_view str) -> std::expected<Position, ParseError> {
      Tokenizer it{str};
      const std::string_view board = it.next();
      const std::string_view color = it.next();
      const std::string_view castling = it.next();
      const std::string_view enpassant = it.next();
      const std::string_view irreversible_clock = it.next();
      const std::string_view ply = it.next();
      if (!it.rest().empty())
        return std::unexpected(ParseError::invalid_length);
      return parse(board, color, castling, enpassant, irreversible_clock, ply);
    }

    static auto parse(std::string_view board, std::string_view color, std::string_view castle, std::string_view enpassant,
                      std::string_view irreversible_clock, std::string_view ply) -> std::expected<Position, ParseError>;

    constexpr auto operator==(const Position &) const -> bool = default;
  };

} // namespace rose

template <> struct std::formatter<rose::Position, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(const rose::Position &position, FmtContext &ctx) const -> FmtContext::iterator {
    using namespace rose;

    int blanks = 0;
    const auto emit_blanks = [&] {
      if (blanks != 0) {
        ctx.advance_to(std::format_to(ctx.out(), "{}", blanks));
        blanks = 0;
      }
    };

    for (int i = 0; i < 64; i++) {
      const int sq = (i + (i & 0b111000)) ^ 0x70;
      const Place p = position.board.m[sq];

      if (p.isEmpty()) {
        blanks++;
      } else {
        emit_blanks();
        ctx.advance_to(std::format_to(ctx.out(), "{}", p));
      }

      if (i % 8 == 7) {
        emit_blanks();
        if (i < 63)
          ctx.advance_to(std::format_to(ctx.out(), "/"));
      }
    }

    ctx.advance_to(std::format_to(ctx.out(), " {} ", position.active_color));

    const Castling castling_rights = position.castlingRights();
    if (castling_rights == Castling::none)
      ctx.advance_to(std::format_to(ctx.out(), "-"));
    if ((castling_rights & Castling::wk) != Castling::none)
      ctx.advance_to(std::format_to(ctx.out(), "K"));
    if ((castling_rights & Castling::wq) != Castling::none)
      ctx.advance_to(std::format_to(ctx.out(), "Q"));
    if ((castling_rights & Castling::bk) != Castling::none)
      ctx.advance_to(std::format_to(ctx.out(), "k"));
    if ((castling_rights & Castling::bq) != Castling::none)
      ctx.advance_to(std::format_to(ctx.out(), "q"));

    if (position.enpassant.isValid()) {
      ctx.advance_to(std::format_to(ctx.out(), " {} ", position.enpassant));
    } else {
      ctx.advance_to(std::format_to(ctx.out(), " - "));
    }

    return std::format_to(ctx.out(), "{} {}", position.irreversible_clock, position.ply / 2 + 1);
  }
};
