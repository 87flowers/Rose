#include "rose/position.hpp"

#include "rose/common.hpp"
#include "rose/square.hpp"
#include "rose/util/string.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace rose {

  auto Position::parse(std::string_view board_str,
                       std::string_view color_str,
                       std::string_view castle_str,
                       std::string_view enpassant_str,
                       std::string_view clock_50mr_str,
                       std::string_view ply_str) -> std::expected<Position, ParseError> {
    Position result {};

    // Parse board
    {
      usize place_index = 0, i = 0;
      std::array<u8, 2> id {{0x01, 0x01}};
      for (; place_index < 64 && i < board_str.size(); i++) {
        const i8 file = static_cast<i8>(place_index % 8);
        const i8 rank = static_cast<i8>(7 - place_index / 8);
        const Square sq = Square::from_file_and_rank(file, rank);
        const char ch = board_str[i];
        if (ch == '/') {
          if (file != 0 || place_index == 0)
            return std::unexpected(ParseError::invalid_char);
        } else if (ch >= '1' and ch <= '8') {
          const usize spaces = ch - '0';
          if (file + spaces > 8)
            return std::unexpected(ParseError::invalid_char);
          place_index += spaces;
        } else if (ch == 'k' || ch == 'K') {
          const usize color = ch == 'k';
          const u8 current_id = 0x00;
          if (result.m_piece_list_sq[color].m[current_id].is_valid())
            return std::unexpected(ParseError::too_many_kings);
          result.m_board[sq] = Place::from(static_cast<Color::Underlying>(color), PieceType::k, current_id);
          result.m_piece_list_sq[color].m[current_id] = sq;
          result.m_piece_list_ptype[color].m[current_id] = PieceType::k;
          place_index++;
        } else if (const auto p = PieceType::parse(ch); p) {
          const auto [pt, c] = *p;
          const usize color = c.to_index();
          u8& current_id = id[color];
          if (current_id >= result.m_piece_list_sq[color].m.size())
            return std::unexpected(ParseError::too_many_pieces);
          result.m_board[sq] = Place::from(c, pt, current_id);
          result.m_piece_list_sq[color].m[current_id] = sq;
          result.m_piece_list_ptype[color].m[current_id] = pt;
          place_index++;
          current_id++;
        } else {
          return std::unexpected(ParseError::invalid_char);
        }
      }
      if (place_index != 64 || i != board_str.size())
        return std::unexpected(ParseError::invalid_length);
    }

    // Parse Color
    {
      if (color_str.size() != 1)
        return std::unexpected(ParseError::invalid_length);
      switch (color_str[0]) {
      case 'b':
        result.m_stm = Color::black;
        break;
      case 'w':
        result.m_stm = Color::white;
        break;
      default:
        return std::unexpected(ParseError::invalid_char);
      }
    }

    // Parse castling rights
    if (castle_str != "-") {
      for (const char ch : castle_str) {
        const auto castle_rights = [&](Color color, i8 rook_file) -> std::optional<ParseError> {
          const Square rook_sq = Square::from_file_and_rank(rook_file, color.to_back_rank());
          const Square king_sq = result.king_sq(color);
          const Place rook_place = result.m_board[rook_sq];
          if (rook_place.color() != color || rook_place.ptype() != PieceType::r || king_sq.rank() != color.to_back_rank())
            return ParseError::invalid_board;
          if (rook_file < king_sq.file())
            result.m_rook_info.set_aside(color, rook_sq);
          if (rook_file > king_sq.file())
            result.m_rook_info.set_hside(color, rook_sq);
          return std::nullopt;
        };
        const auto scan_for_rook = [&](Color color, int file, int direction) -> std::optional<ParseError> {
          while (true) {
            if (file < 0 || file > 7)
              return ParseError::invalid_board;
            const Square rook_sq = Square::from_file_and_rank(static_cast<u8>(file), color.to_back_rank());
            const Place rook_place = result.m_board[rook_sq];
            if (rook_place.isEmpty()) {
              file += direction;
              continue;
            }
            if (rook_place.color() != color || rook_place.ptype() != PieceType::r)
              return ParseError::invalid_board;
            return castle_rights(color, file);
          }
        };
        if (ch >= 'A' && ch <= 'H') {
          if (const auto err = castle_rights(Color::white, ch - 'A'); err)
            return std::unexpected(*err);
        } else if (ch >= 'a' && ch <= 'h') {
          if (const auto err = castle_rights(Color::black, ch - 'a'); err)
            return std::unexpected(*err);
        } else if (ch == 'Q') {
          if (const auto err = scan_for_rook(Color::white, 0, +1); err)
            return std::unexpected(*err);
        } else if (ch == 'K') {
          if (const auto err = scan_for_rook(Color::white, 7, -1); err)
            return std::unexpected(*err);
        } else if (ch == 'q') {
          if (const auto err = scan_for_rook(Color::black, 0, +1); err)
            return std::unexpected(*err);
        } else if (ch == 'k') {
          if (const auto err = scan_for_rook(Color::black, 7, -1); err)
            return std::unexpected(*err);
        } else {
          return std::unexpected(ParseError::invalid_char);
        }
      }
    }

    // Parse enpassant square
    if (enpassant_str != "-") {
      const auto enpassant = Square::parse(enpassant_str);
      if (!enpassant)
        return std::unexpected(enpassant.error());
      result.m_enpassant = enpassant.value();
    }

    // Parse 50mr clock
    if (const auto clock_50mr = parse_u16(clock_50mr_str); clock_50mr && *clock_50mr <= 200) {
      result.m_50mr = *clock_50mr;
    } else {
      return std::unexpected(ParseError::out_of_range);
    }

    // Parse ply
    if (const auto ply = parse_u16(ply_str); *ply && *ply != 0 && *ply < 10000) {
      result.m_ply = (*ply - 1) * 2 + static_cast<u16>(result.m_stm.to_index());
    } else {
      return std::unexpected(ParseError::out_of_range);
    }

    // result.m_attack_table = result.calcAttacksSlow();
    // result.m_hash = result.calcHashSlow();

    return result;
  }

}  // namespace rose
