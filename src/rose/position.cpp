#include "rose/position.h"

#include <print>

namespace rose {

  const Position Position::startpos = Position::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").value();

  auto Position::castlingRights() const -> Castling {
    Castling result = Castling::none;
    if (board.m[Square::fromFileAndRank(4, 0).raw] == Place::unmoved_wk) {
      if (board.m[Square::fromFileAndRank(0, 0).raw] == Place::unmoved_wr)
        result |= Castling::wq;
      if (board.m[Square::fromFileAndRank(7, 0).raw] == Place::unmoved_wr)
        result |= Castling::wk;
    }
    if (board.m[Square::fromFileAndRank(4, 7).raw] == Place::unmoved_bk) {
      if (board.m[Square::fromFileAndRank(0, 7).raw] == Place::unmoved_br)
        result |= Castling::bq;
      if (board.m[Square::fromFileAndRank(7, 7).raw] == Place::unmoved_br)
        result |= Castling::bk;
    }
    return result;
  }

  auto Position::prettyPrint() const -> void {
    std::print("   +------------------------+\n");
    for (i8 rank = 7; rank >= 0; rank--) {
      std::print(" {} |", static_cast<char>('1' + rank));
      for (u8 file = 0; file < 8; file++) {
        const Square sq = Square::fromFileAndRank(file, static_cast<u8>(rank));
        const Place place = board.m[sq.raw];
        std::print(" {} ", place);
      }
      std::print("|\n");
    }
    std::print("   +------------------------+\n");
    std::print("     a  b  c  d  e  f  g  h  \n");
  }

  auto Position::parse(std::string_view board_str, std::string_view color_str, std::string_view castle_str, std::string_view enpassant_str,
                       std::string_view irreversible_clock_str, std::string_view ply_str) -> std::expected<Position, ParseError> {
    Position result{};

    // Parse board
    {
      usize place_index = 0, i = 0;
      for (; place_index < 64 && i < board_str.size(); i++) {
        const usize file = place_index % 8;
        const usize rank = 7 - place_index / 8;
        const Square sq = Square::fromFileAndRank(file, rank);
        const char ch = board_str[i];
        if (ch == '/') {
          if (file != 0 || place_index == 0)
            return std::unexpected(ParseError::invalid_char);
        } else if (ch >= '1' and ch <= '8') {
          const usize spaces = ch - '0';
          if (file + spaces > 8)
            return std::unexpected(ParseError::invalid_char);
          place_index += spaces;
        } else if (const auto p = Place::parse(ch); p) {
          result.board.m[sq.raw] = *p;
          place_index++;
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
        result.active_color = Color::black;
        break;
      case 'w':
        result.active_color = Color::white;
        break;
      default:
        return std::unexpected(ParseError::invalid_char);
      }
    }

    // Parse castling rights
    if (castle_str != "-") {
      for (const char ch : castle_str) {
        const auto castle_rights = [&](Color color, u8 rook_file) -> std::optional<ParseError> {
          const Square rook_sq = Square::fromFileAndRank(rook_file, color.toBackRank());
          const Square king_sq = Square::fromFileAndRank(4, color.toBackRank());
          if (result.board.m[rook_sq.raw].color() != color || result.board.m[rook_sq.raw].ptype() != PieceType::r ||
              result.board.m[king_sq.raw].color() != color || result.board.m[king_sq.raw].ptype() != PieceType::k)
            return ParseError::invalid_board;
          result.board.m[rook_sq.raw] = result.board.m[rook_sq.raw].toPristine();
          result.board.m[king_sq.raw] = result.board.m[king_sq.raw].toPristine();
          return std::nullopt;
        };
        switch (ch) {
        case 'K':
        case 'H':
          if (const auto err = castle_rights(Color::white, 7); err)
            return std::unexpected(*err);
          break;
        case 'Q':
        case 'A':
          if (const auto err = castle_rights(Color::white, 0); err)
            return std::unexpected(*err);
          break;
        case 'k':
        case 'h':
          if (const auto err = castle_rights(Color::black, 7); err)
            return std::unexpected(*err);
          break;
        case 'q':
        case 'a':
          if (const auto err = castle_rights(Color::black, 0); err)
            return std::unexpected(*err);
          break;
        default:
          return std::unexpected(ParseError::invalid_char);
        }
      }
    }

    // Parse enpassant square
    if (enpassant_str != "-") {
      const auto enpassant = Square::parse(enpassant_str);
      if (!enpassant)
        return std::unexpected(enpassant.error());
      result.enpassant = enpassant.value();
    }

    // Parse irreversible clock
    if (const usize irreversible_clock = std::stoi(std::string{irreversible_clock_str}); irreversible_clock <= 200) {
      result.irreversible_clock = irreversible_clock;
    } else {
      return std::unexpected(ParseError::out_of_range);
    }

    // Parse ply
    if (const usize ply = std::stoi(std::string{ply_str}); ply != 0 && ply < 10000) {
      result.ply = (ply - 1) * 2 + static_cast<u16>(result.active_color.toIndex());
    } else {
      return std::unexpected(ParseError::out_of_range);
    }

    return result;
  }

} // namespace rose
