#pragma once

#include "rose/bitboard.hpp"
#include "rose/board.hpp"
#include "rose/common.hpp"
#include "rose/config.hpp"
#include "rose/move.hpp"
#include "rose/square.hpp"
#include "rose/util/tokenizer.hpp"

#include <fmt/format.h>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rose {

  template<typename T>
  struct alignas(16) PieceList {
    std::array<T, 16> m {};

    static_assert(sizeof(T) == sizeof(u8));

    auto to_vector() const -> u8x16 {
      return u8x16::load(m.data());
    }

    template<PieceType... ptype>
    auto piece_mask_for() const -> PieceMask
      requires std::is_same_v<T, PieceType>
    {
      static_assert(sizeof...(ptype) > 0);
      if constexpr (sizeof...(ptype) == 1) {
        return PieceMask {to_vector().eq(u8x16::splat(ptype.raw...)).to_bits()};
      } else {
        constexpr u8 test_bits = (0 | ... | (1 << ptype.to_index()));
        constexpr u8x16 ptype_to_bits {{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0, 0, 0, 0, 0, 0, 0, 0}};
        return PieceMask {(to_vector().swizzle(ptype_to_bits) & u8x16::splat(test_bits)).nonzeros().to_bits()};
      }
    }

    constexpr auto operator[](PieceId id) -> T& {
      return m[id.raw];
    }

    constexpr auto operator[](PieceId id) const -> T {
      return m[id.raw];
    }

    constexpr auto operator==(const PieceList& other) const -> bool {
      return m == other.m;
    }
  };

  struct RookInfo {
    u32 raw = 0x80808080;

    constexpr auto half(Color color) const -> u16 {
      return static_cast<u16>(raw >> (color.to_index() * 16));
    }

    constexpr auto aside(Color color) const -> Square {
      return Square {static_cast<u8>(half(color))};
    }

    constexpr auto hside(Color color) const -> Square {
      return Square {static_cast<u8>(half(color) >> 8)};
    }

    constexpr auto set_aside(Color color, Square sq) -> void {
      raw &= ~(static_cast<u32>(0x00FF) << (color.to_index() * 16));
      raw |= static_cast<u32>(sq.raw) << (color.to_index() * 16);
    }

    constexpr auto set_hside(Color color, Square sq) -> void {
      raw &= ~(static_cast<u32>(0xFF00) << (color.to_index() * 16));
      raw |= static_cast<u32>(sq.raw) << (color.to_index() * 16 + 8);
    }

    constexpr auto clear(Color color) -> void {
      raw |= static_cast<u32>(0x8080) << (color.to_index() * 16);
    }

    constexpr auto to_vector() const -> u8x16 {
#if LPS_SSE4_2 || LPS_AVX2 || LPS_AVX512
      return u8x16 {_mm_cvtsi32_si128(std::bit_cast<int>(raw))};
#else
      u8x16 result {};
      std::memcpy(&result, &raw, sizeof(u32));
      return result;
#endif
    }

    constexpr auto unset(Color color, Square sq) -> void {
      const u8x16 x = to_vector();
      const m8x16 mask = x.eq(u8x16::splat(sq.raw));
      const u8x16 y = mask.select(x, u8x16::splat(0x80));
#if LPS_SSE4_2 || LPS_AVX2 || LPS_AVX512
      raw = std::bit_cast<u32>(_mm_cvtsi128_si32(y.raw));
#else
      raw = y.extract_aligned<u32, 0>();
#endif
    }

    constexpr auto has(Square sq) const -> bool {
      return to_vector().eq(u8x16::splat(sq.raw)).to_bits() & 0b1111;
    }

    constexpr auto is_clear() const -> bool {
      return to_index() == 0;
    }

    constexpr auto to_index() const -> usize {
#if LPS_AVX2 || LPS_AVX512
      return _pext_u32(~raw, 0x80808080);
#else
      return ((~raw & 0x80808080) * 0x00204081) >> 28;
#endif
    }

    constexpr auto operator==(const RookInfo&) const -> bool = default;
  };

  struct Position {
  private:
    friend class fmt::formatter<Position, char>;

    std::array<Wordboard, 2> m_attack_table {};
    Byteboard m_board {};
    std::array<PieceList<Square>, 2> m_piece_list_sq {};
    std::array<PieceList<PieceType>, 2> m_piece_list_ptype {};
    RookInfo m_rook_info {};
    u16 m_50mr {};
    u16 m_ply {};
    Square m_enpassant = Square::invalid();
    Color m_stm {};

    auto add_attacks(Color color, Square sq, PieceId id, PieceType ptype) -> void;
    auto remove_attacks(Color color, PieceId id) -> void;
    auto toggle_sliders_single(Square sq) -> void;

    template<bool update_sliders>
    auto add_piece(Color color, Square sq, PieceId id, PieceType ptype) -> void;
    template<bool update_sliders>
    auto remove_piece(Color color, Square sq, PieceId id) -> void;
    template<bool is_capture>
    auto move_piece(Color color, Square src, Square dst, PieceId id, PieceType src_ptype, PieceType dst_ptype) -> void;

  public:
    static auto startpos() -> Position;

    constexpr Position() = default;

    constexpr auto board() const -> const Byteboard& {
      return m_board;
    }

    constexpr auto attack_table(Color color) const -> const Wordboard& {
      return m_attack_table[color.to_index()];
    }

    constexpr auto piece_list_sq(Color color) const -> PieceList<Square> {
      return m_piece_list_sq[color.to_index()];
    }

    constexpr auto piece_list_type(Color color) const -> PieceList<PieceType> {
      return m_piece_list_ptype[color.to_index()];
    }

    constexpr auto rook_info() const -> RookInfo {
      return m_rook_info;
    }

    constexpr auto fifty_move_clock() const -> u16 {
      return m_50mr;
    }

    constexpr auto full_move_counter() const -> u16 {
      return m_ply / 2 + 1;
    }

    constexpr auto enpassant() const -> Square {
      return m_enpassant;
    }

    constexpr auto stm() const -> Color {
      return m_stm;
    }

    auto king_sq(Color color) const -> Square {
      return m_piece_list_sq[color.to_index()][PieceId::king()];
    }

    template<PieceType... ptypes>
    auto piece_mask_for(Color color) const -> PieceMask {
      return piece_list_type(color).piece_mask_for<ptypes...>();
    }

    template<PieceType ptype>
    auto bitboard_for(Color color) const -> Bitboard {
      return board().bitboard_for<ptype>(color);
    }

    auto is_valid() const -> bool {
      return attack_table(m_stm).read(king_sq(!m_stm)).is_empty();
    }

    auto move(Move m) const -> Position;

    auto calc_pin_info() const -> std::tuple<std::array<PieceMask, 64>, Bitboard>;

    auto calc_attacks_slow() const -> std::array<Wordboard, 2>;
    auto calc_attacks_slow(Square sq) const -> std::array<PieceMask, 2>;

    static auto parse(std::string_view str) -> std::expected<Position, ParseError> {
      Tokenizer it {str};
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

    static auto parse(std::string_view board,
                      std::string_view color,
                      std::string_view castle,
                      std::string_view enpassant,
                      std::string_view clock_50mr,
                      std::string_view ply) -> std::expected<Position, ParseError>;

    auto dump() const -> void;

    constexpr auto operator==(const Position&) const -> bool = default;
  };

}  // namespace rose

template<>
struct fmt::formatter<rose::Position, char> {
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
    return ctx.begin();
  }

  template<class FmtContext>
  auto format(const rose::Position& position, FmtContext& ctx) const -> FmtContext::iterator {
    using namespace rose;

    int blanks = 0;
    const auto emit_blanks = [&] {
      if (blanks != 0) {
        ctx.advance_to(fmt::format_to(ctx.out(), "{}", blanks));
        blanks = 0;
      }
    };

    for (i8 rank = 7; rank >= 0; rank--) {
      for (i8 file = 0; file < 8; file++) {
        const Square sq = Square::from_file_and_rank(file, rank);
        const Place p = position.m_board[sq];
        if (p.is_empty()) {
          blanks++;
        } else {
          emit_blanks();
          ctx.advance_to(fmt::format_to(ctx.out(), "{}", p));
        }
      }
      emit_blanks();
      if (rank != 0) {
        ctx.advance_to(fmt::format_to(ctx.out(), "/"));
      }
    }

    ctx.advance_to(fmt::format_to(ctx.out(), " {} ", position.m_stm));

    const RookInfo rook_info = position.rook_info();
    if (rook_info.is_clear()) {
      ctx.advance_to(fmt::format_to(ctx.out(), "-"));
    }
    if (rook_info.hside(Color::white).is_valid()) {
      const bool is_classical = !config::frc && rook_info.hside(Color::white).file() == 7;
      ctx.advance_to(fmt::format_to(ctx.out(), "{}", is_classical ? 'K' : static_cast<char>('A' + rook_info.hside(Color::white).file())));
    }
    if (rook_info.aside(Color::white).is_valid()) {
      const bool is_classical = !config::frc && rook_info.aside(Color::white).file() == 0;
      ctx.advance_to(fmt::format_to(ctx.out(), "{}", is_classical ? 'Q' : static_cast<char>('A' + rook_info.aside(Color::white).file())));
    }
    if (rook_info.hside(Color::black).is_valid()) {
      const bool is_classical = !config::frc && rook_info.hside(Color::black).file() == 7;
      ctx.advance_to(fmt::format_to(ctx.out(), "{}", is_classical ? 'k' : static_cast<char>('a' + rook_info.hside(Color::black).file())));
    }
    if (rook_info.aside(Color::black).is_valid()) {
      const bool is_classical = !config::frc && rook_info.aside(Color::black).file() == 0;
      ctx.advance_to(fmt::format_to(ctx.out(), "{}", is_classical ? 'q' : static_cast<char>('a' + rook_info.aside(Color::black).file())));
    }

    if (position.m_enpassant.is_valid()) {
      ctx.advance_to(fmt::format_to(ctx.out(), " {} ", position.m_enpassant));
    } else {
      ctx.advance_to(fmt::format_to(ctx.out(), " - "));
    }

    return fmt::format_to(ctx.out(), "{} {}", position.m_50mr, position.full_move_counter());
  }
};
