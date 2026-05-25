#pragma once

#include "rose/bitboard.hpp"
#include "rose/board.hpp"
#include "rose/common.hpp"
#include "rose/hash.hpp"
#include "rose/move.hpp"
#include "rose/score.hpp"
#include "rose/square.hpp"
#include "rose/util/tokenizer.hpp"

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

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
    std::array<Wordboard, 2> m_attack_table {};
    Byteboard m_board {};
    std::array<PieceList<Square>, 2> m_piece_list_sq {};
    std::array<PieceList<PieceType>, 2> m_piece_list_ptype {};
    u64 m_hash {};
    RookInfo m_rook_info {};
    u16 m_50mr {};
    u16 m_ply {};
    Square m_enpassant = Square::invalid();
    Color m_stm {};

    mutable std::array<PieceMask, 64> m_cached_masked_attack_table {};
    mutable Bitboard m_cached_pinned {};
    mutable bool m_valid_pin_info = false;

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

    constexpr auto hash() const -> Hash {
      return m_hash;
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

    auto place_at(Square sq) const -> Place {
      rose_assert(sq.is_valid());
      return m_board.mailbox[sq.raw];
    }

    auto ptype_at(Square sq) const -> PieceType {
      return place_at(sq).ptype();
    }

    template<PieceType... ptypes>
    auto piece_mask_for(Color color) const -> PieceMask {
      return piece_list_type(color).piece_mask_for<ptypes...>();
    }

    template<PieceType ptype>
    auto piece_count() const -> int {
      return piece_list_type(Color::white).piece_mask_for<ptype>().popcount() + piece_list_type(Color::black).piece_mask_for<ptype>().popcount();
    }

    template<PieceType ptype>
    auto bitboard_for(Color color) const -> Bitboard {
      return board().bitboard_for<ptype>(color);
    }

    auto is_valid() const -> bool {
      return attack_table(m_stm).read(king_sq(!m_stm)).is_empty();
    }

    auto is_in_check() const -> bool {
      return !attack_table(!m_stm).read(king_sq(m_stm)).is_empty();
    }

    auto is_legal_slow(Move m) const -> bool;
    auto has_no_legal_moves_slow() const -> bool;
    auto is_stalemate_slow() const -> bool;
    auto is_fifty_move_draw(i32 ply = 0) const -> std::optional<Score>;
    auto is_repetition(const std::vector<u64>& hash_stack, usize hash_waterline) const -> bool;

    auto move(Move m) const -> Position;
    auto null_move() const -> Position;

    auto calc_hash_slow() const -> Hash;

    auto pin_info() const -> std::tuple<const std::array<PieceMask, 64>&, Bitboard>;
    auto calc_pin_info_slow() const -> std::tuple<std::array<PieceMask, 64>, Bitboard>;

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

    auto to_string(MoveFormat format) const -> std::string;

    constexpr auto operator==(const Position&) const -> bool = default;
  };

}  // namespace rose
