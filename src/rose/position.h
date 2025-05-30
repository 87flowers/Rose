#pragma once

#include <array>
#include <bit>
#include <format>
#include <optional>
#include <utility>
#include <vector>

#include "rose/byteboard.h"
#include "rose/common.h"
#include "rose/config.h"
#include "rose/move.h"
#include "rose/square.h"
#include "rose/util/tokenizer.h"
#include "rose/util/types.h"
#include "rose/util/vec.h"

namespace rose {

  template <typename T> union alignas(16) PieceList {
    v128 x;
    std::array<T, 16> m{};

    auto maskEq(T value) const -> u16 { return vec::eq8(x, v128::broadcast8(value.raw)); }

    auto dump() const -> void;

    constexpr forceinline auto operator[](PieceId id) -> T & { return m[id.raw]; }
    constexpr forceinline auto operator[](PieceId id) const -> T { return m[id.raw]; }

    constexpr auto operator==(const PieceList &other) const -> bool { return m == other.m; }
  };

  struct RookInfo {
    Square aside = Square::invalid();
    Square hside = Square::invalid();

    constexpr auto clear() -> void {
      aside = Square::invalid();
      hside = Square::invalid();
    }

    constexpr auto unset(Square sq) -> void {
      aside = aside == sq ? Square::invalid() : aside;
      hside = hside == sq ? Square::invalid() : hside;
    }

    constexpr auto isClear() const -> bool { return !aside.isValid() && !hside.isValid(); }

    constexpr auto operator==(const RookInfo &) const -> bool = default;
  };

  struct Position {
  private:
    friend class std::formatter<rose::Position, char>;

    std::array<Wordboard, 2> m_attack_table{};
    std::array<PieceList<Square>, 2> m_piece_list_sq{};
    std::array<PieceList<PieceType>, 2> m_piece_list_ptype{};
    Byteboard m_board{};
    u64 m_hash{};
    u16 m_50mr{};
    u16 m_ply{};
    Color m_active_color{};
    Square m_enpassant = Square::invalid();
    std::array<RookInfo, 2> m_rook_info;

  public:
    static auto startpos() -> Position;

    constexpr Position() = default;

    constexpr auto board() const -> const Byteboard & { return m_board; }
    constexpr auto attackTable(Color color) const -> const Wordboard & { return m_attack_table[color.toIndex()]; }
    constexpr auto pieceListSq(Color color) const -> PieceList<Square> { return m_piece_list_sq[color.toIndex()]; }
    constexpr auto pieceListType(Color color) const -> PieceList<PieceType> { return m_piece_list_ptype[color.toIndex()]; }
    constexpr auto activeColor() const -> Color { return m_active_color; }
    constexpr auto hash() const -> u64 { return m_hash; }
    constexpr auto fiftyMoveClock() const -> u16 { return m_50mr; }

    auto kingSq(Color color) const -> Square { return m_piece_list_sq[color.toIndex()].m[0]; }
    auto enpassant() const -> Square { return m_enpassant; }
    auto rookInfo(Color color) const -> RookInfo { return m_rook_info[color.toIndex()]; }

    auto isValid() const -> bool { return attackTable(m_active_color).r[kingSq(m_active_color.invert()).raw] == 0; }
    auto isInCheck() const -> bool { return attackTable(m_active_color.invert()).r[kingSq(m_active_color).raw] != 0; }

    auto isDraw(const std::vector<u64> &hash_stack) const -> std::optional<i32>;
    auto isDraw(const std::vector<u64> &hash_stack, usize hash_waterline, i32 ply) const -> std::optional<i32>;
    auto isRepetition(const std::vector<u64> &hash_stack, usize hash_waterline) const -> bool;

    auto moveNull() const -> Position;
    auto move(Move m) const -> Position;

    auto calcAttacksSlow() const -> std::array<Wordboard, 2>;
    auto calcAttacksSlow(Square sq) const -> std::array<u16, 2>;
    auto calcHashSlow() const -> u64;

    auto prettyPrint() const -> void;
    auto printAttackTable() const -> void;

    static auto parse(std::string_view str) -> std::expected<Position, ParseError> {
      Tokenizer it{str};
      const std::string_view board = it.next();
      const std::string_view color = it.next();
      const std::string_view castling = it.next();
      const std::string_view enpassant = it.next();
      const std::string_view clock_50mr = it.next();
      const std::string_view ply = it.next();
      if (!it.rest().empty())
        return std::unexpected(ParseError::invalid_length);
      return parse(board, color, castling, enpassant, clock_50mr, ply);
    }

    static auto parse(std::string_view board, std::string_view color, std::string_view castle, std::string_view enpassant,
                      std::string_view clock_50mr, std::string_view ply) -> std::expected<Position, ParseError>;

    constexpr auto operator==(const Position &) const -> bool = default;

  private:
    template <bool update_to_silders = true, PieceType dest_ptype = PieceType::none>
    forceinline auto movePiece(bool color, Square from, Square to, PieceId id, PieceType ptype) -> void;
    forceinline auto incrementalSliderUpdate(Square sq) -> void;
    forceinline auto removeAttacks(bool color, PieceId id) -> void;
    forceinline auto addAttacks(bool color, Square sq, PieceId id, PieceType ptype) -> void;
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

    const RookInfo white_rook_info = position.rookInfo(Color::white);
    const RookInfo black_rook_info = position.rookInfo(Color::black);
    if (white_rook_info.isClear() && black_rook_info.isClear())
      ctx.advance_to(std::format_to(ctx.out(), "-"));
    if (white_rook_info.hside.isValid()) {
      const bool is_classical = !config::frc && white_rook_info.hside.file() == 7;
      ctx.advance_to(std::format_to(ctx.out(), "{}", is_classical ? 'K' : static_cast<char>('A' + white_rook_info.hside.file())));
    }
    if (white_rook_info.aside.isValid()) {
      const bool is_classical = !config::frc && white_rook_info.aside.file() == 0;
      ctx.advance_to(std::format_to(ctx.out(), "{}", is_classical ? 'Q' : static_cast<char>('A' + white_rook_info.aside.file())));
    }
    if (black_rook_info.hside.isValid()) {
      const bool is_classical = !config::frc && black_rook_info.hside.file() == 7;
      ctx.advance_to(std::format_to(ctx.out(), "{}", is_classical ? 'k' : static_cast<char>('a' + black_rook_info.hside.file())));
    }
    if (black_rook_info.aside.isValid()) {
      const bool is_classical = !config::frc && black_rook_info.aside.file() == 0;
      ctx.advance_to(std::format_to(ctx.out(), "{}", is_classical ? 'q' : static_cast<char>('a' + black_rook_info.aside.file())));
    }

    if (position.m_enpassant.isValid()) {
      ctx.advance_to(std::format_to(ctx.out(), " {} ", position.m_enpassant));
    } else {
      ctx.advance_to(std::format_to(ctx.out(), " - "));
    }

    return std::format_to(ctx.out(), "{} {}", position.m_50mr, position.m_ply / 2 + 1);
  }
};
