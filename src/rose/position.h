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

  template <typename T> struct alignas(16) PieceList {
    std::array<T, 16> m{};

    static_assert(sizeof(T) == sizeof(u8));
    forceinline auto x() const -> v128 { return v128::load(m.data()); }

    auto valid() const -> u16 { return x().nonzero8(); }
    auto maskEq(T value) const -> u16 { return vec::eq8(x(), v128::broadcast8(value.raw)); }

    auto dump() const -> void;

    constexpr forceinline auto operator[](PieceId id) -> T & { return m[id.raw]; }
    constexpr forceinline auto operator[](PieceId id) const -> T { return m[id.raw]; }

    constexpr auto operator==(const PieceList &other) const -> bool { return m == other.m; }
  };

  struct RookInfo {
    u32 raw = 0x80808080;

    constexpr auto half(Color color) const -> u16 { return static_cast<u16>(raw >> (color.toIndex() * 16)); }
    constexpr auto aside(Color color) const -> Square { return Square{static_cast<u8>(half(color))}; }
    constexpr auto hside(Color color) const -> Square { return Square{static_cast<u8>(half(color) >> 8)}; }
    constexpr auto setAside(Color color, Square sq) -> void {
      raw &= ~(static_cast<u32>(0x00FF) << (color.toIndex() * 16));
      raw |= static_cast<u32>(sq.raw) << (color.toIndex() * 16);
    }
    constexpr auto setHside(Color color, Square sq) -> void {
      raw &= ~(static_cast<u32>(0xFF00) << (color.toIndex() * 16));
      raw |= static_cast<u32>(sq.raw) << (color.toIndex() * 16 + 8);
    }

    constexpr auto clear(Color color) -> void { raw |= static_cast<u32>(0x8080) << (color.toIndex() * 16); }

    constexpr auto unset(Color color, Square sq) -> void {
      const v128 x = v128::from32(raw);
      const u16 mask = vec::eq8(x, v128::broadcast8(sq.raw));
      const v128 y = vec::blend8(mask, x, v128::broadcast8(0x80));
      raw = static_cast<u32>(y.to32());
    }

    constexpr auto has(Square sq) const -> bool { return vec::eq8(v128::from32(raw), v128::broadcast8(sq.raw)) & 0b1111; }

    constexpr auto isClear() const -> bool { return toIndex() == 0; }

    constexpr auto toIndex() const -> usize { return _pext_u32(~raw, 0x80808080); }

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
    RookInfo m_rook_info{};

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
    constexpr auto fullMoveCounter() const -> u16 { return m_ply / 2 + 1; }

    auto kingSq(Color color) const -> Square { return m_piece_list_sq[color.toIndex()].m[0]; }
    auto enpassant() const -> Square { return m_enpassant; }
    auto rookInfo() const -> RookInfo { return m_rook_info; }

    auto pieceOn(Square sq) const -> PieceType { return m_board.read(sq).ptype(); }

    auto isValid() const -> bool { return attackTable(m_active_color).read(kingSq(m_active_color.invert())) == 0; }
    auto isInCheck() const -> bool { return attackTable(m_active_color.invert()).read(kingSq(m_active_color)) != 0; }

    auto isRuleDraw(const std::vector<u64> &hash_stack) const -> std::optional<i32>;
    auto isRuleDraw(const std::vector<u64> &hash_stack, usize hash_waterline, i32 ply) const -> std::optional<i32>;
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
        const Place p = position.m_board.read(sq);

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

    const RookInfo rook_info = position.rookInfo();
    if (rook_info.isClear())
      ctx.advance_to(std::format_to(ctx.out(), "-"));
    if (rook_info.hside(Color::white).isValid()) {
      const bool is_classical = !config::frc && rook_info.hside(Color::white).file() == 7;
      ctx.advance_to(std::format_to(ctx.out(), "{}", is_classical ? 'K' : static_cast<char>('A' + rook_info.hside(Color::white).file())));
    }
    if (rook_info.aside(Color::white).isValid()) {
      const bool is_classical = !config::frc && rook_info.aside(Color::white).file() == 0;
      ctx.advance_to(std::format_to(ctx.out(), "{}", is_classical ? 'Q' : static_cast<char>('A' + rook_info.aside(Color::white).file())));
    }
    if (rook_info.hside(Color::black).isValid()) {
      const bool is_classical = !config::frc && rook_info.hside(Color::black).file() == 7;
      ctx.advance_to(std::format_to(ctx.out(), "{}", is_classical ? 'k' : static_cast<char>('a' + rook_info.hside(Color::black).file())));
    }
    if (rook_info.aside(Color::black).isValid()) {
      const bool is_classical = !config::frc && rook_info.aside(Color::black).file() == 0;
      ctx.advance_to(std::format_to(ctx.out(), "{}", is_classical ? 'q' : static_cast<char>('a' + rook_info.aside(Color::black).file())));
    }

    if (position.m_enpassant.isValid()) {
      ctx.advance_to(std::format_to(ctx.out(), " {} ", position.m_enpassant));
    } else {
      ctx.advance_to(std::format_to(ctx.out(), " - "));
    }

    return std::format_to(ctx.out(), "{} {}", position.m_50mr, position.fullMoveCounter());
  }
};
