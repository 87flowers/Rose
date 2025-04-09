#pragma once

#include <array>
#include <bit>
#include <format>
#include <utility>

#include "rose/byteboard.h"
#include "rose/common.h"
#include "rose/square.h"
#include "rose/util/tokenizer.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  union alignas(16) PieceList {
    v128 x;
    std::array<Square, 16> m{};

    auto dump() const -> void;

    constexpr auto operator==(const PieceList &other) const -> bool { return m == other.m; }
  };

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

    std::array<Wordboard, 2> m_attack_table{};
    std::array<PieceList, 2> m_piece_list{};
    Byteboard m_board{};
    u64 m_pinned{};
    u64 m_hash{};
    u16 m_ply{};
    u16 m_irreversible_clock{};
    Color m_active_color{};
    Square m_enpassant = Square::invalid();

  public:
    static const Position startpos;

    constexpr Position() = default;

    constexpr auto board() const -> const Byteboard & { return m_board; }
    constexpr auto attackTable(Color color) const -> const Wordboard & { return m_attack_table[color.toIndex()]; }
    constexpr auto pieceList(Color color) const -> PieceList { return m_piece_list[color.toIndex()]; }
    constexpr auto activeColor() const -> Color { return m_active_color; }
    constexpr auto pinned() const -> u64 { return m_pinned; }

    auto kingSq(Color color) const -> Square { return Square{static_cast<u8>(std::countr_zero(m_board.bitboardFor<PieceType::k>(color)))}; }
    auto castlingRights() const -> Castling;

    auto calcPinnedSlow(Color active_color, Square king_sq) const -> u64;

    auto calcAttacksSlow() const -> std::array<Wordboard, 2>;
    auto calcAttacksSlow(Square sq) const -> std::array<u16, 2>;

    auto prettyPrint() const -> void;
    auto printAttackTable() const -> void;

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

    for (i8 rank = 7; rank >= 0; rank--) {
      for (u8 file = 0; file < 8; file++) {
        const Square sq = Square::fromFileAndRank(file, static_cast<u8>(rank));
        const Place p = position.m_board.m[sq.raw];

        if (p.isEmpty()) {
          blanks++;
        } else {
          emit_blanks();
          ctx.advance_to(std::format_to(ctx.out(), "{}", p));
        }

        if (file == 7) {
          emit_blanks();
          if (rank != 0)
            ctx.advance_to(std::format_to(ctx.out(), "/"));
        }
      }
    }

    ctx.advance_to(std::format_to(ctx.out(), " {} ", position.m_active_color));

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

    if (position.m_enpassant.isValid()) {
      ctx.advance_to(std::format_to(ctx.out(), " {} ", position.m_enpassant));
    } else {
      ctx.advance_to(std::format_to(ctx.out(), " - "));
    }

    return std::format_to(ctx.out(), "{} {}", position.m_irreversible_clock, position.m_ply / 2 + 1);
  }
};
