#include "rose/position.h"

#include <bit>
#include <print>
#include <utility>

namespace rose {

  template <> auto PieceList<Square>::dump() const -> void {
    for (const Square sq : m) {
      if (sq.isValid()) {
        std::print("{} ", sq);
      } else {
        std::print("-- ");
      }
    }
    std::print("\n");
  }

  template <> auto PieceList<PieceType>::dump() const -> void {
    for (const PieceType ptype : m) {
      std::print("{} ", ptype);
    }
    std::print("\n");
  }

  const Position Position::startpos = Position::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").value();

  auto Position::move(Move m) const -> Position {
    Position new_pos = *this;

    const Square from = m.from();
    const Square to = m.to();
    const u8 src_id = m_id.r[from.raw];
    const u8 dest_id = m_id.r[to.raw];
    const Place src_place = m_board.m[from.raw];
    const Place dest_place = m_board.m[to.raw];
    const bool color = m_active_color.toIndex();

    if (m_enpassant.isValid()) {
      // TODO: m_hash
      new_pos.m_enpassant = Square::invalid();
    }

    const auto check_src_castling_rights = [&] {
      if (src_place.ptype() == PieceType::r) {
        new_pos.m_rook_info[color].unset(from);
      } else if (src_place.ptype() == PieceType::k) {
        new_pos.m_rook_info[color].clear();
      }
    };

    const auto check_dest_castling_rights = [&] {
      if (dest_place.ptype() == PieceType::r) {
        new_pos.m_rook_info[color].unset(to);
      }
    };

    const auto normal = [&] {
      new_pos.m_piece_list_sq[color].m[src_id] = to;
      new_pos.m_board.m[from.raw] = Place::empty;
      new_pos.m_board.m[to.raw] = src_place;
      new_pos.m_id.r[from.raw] = 0x80;
      new_pos.m_id.r[to.raw] = src_id;
      // TODO: m_hash
      if (src_place.ptype() != PieceType::p) {
        new_pos.m_irreversible_clock++;
      } else {
        new_pos.m_irreversible_clock = 0;
      }
      check_src_castling_rights();
    };

    const auto capture = [&] {
      new_pos.m_piece_list_sq[color].m[src_id] = to;
      new_pos.m_piece_list_sq[!color].m[dest_id] = Square::invalid();
      new_pos.m_piece_list_ptype[!color].m[dest_id] = PieceType::none;
      new_pos.m_board.m[from.raw] = Place::empty;
      new_pos.m_board.m[to.raw] = src_place;
      new_pos.m_id.r[from.raw] = 0x80;
      new_pos.m_id.r[to.raw] = src_id;
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
      check_src_castling_rights();
      check_dest_castling_rights();
    };

    const auto promo = [&](PieceType ptype) {
      new_pos.m_piece_list_sq[color].m[src_id] = to;
      new_pos.m_piece_list_ptype[color].m[src_id] = ptype;
      new_pos.m_board.m[from.raw] = Place::empty;
      new_pos.m_board.m[to.raw] = Place::fromColorAndPtype(m_active_color, ptype);
      new_pos.m_id.r[from.raw] = 0x80;
      new_pos.m_id.r[to.raw] = src_id;
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
    };

    const auto cap_promo = [&](PieceType ptype) {
      new_pos.m_piece_list_sq[color].m[src_id] = to;
      new_pos.m_piece_list_sq[!color].m[dest_id] = Square::invalid();
      new_pos.m_piece_list_ptype[color].m[src_id] = ptype;
      new_pos.m_piece_list_ptype[!color].m[dest_id] = PieceType::none;
      new_pos.m_board.m[from.raw] = Place::empty;
      new_pos.m_board.m[to.raw] = Place::fromColorAndPtype(m_active_color, ptype);
      new_pos.m_id.r[from.raw] = 0x80;
      new_pos.m_id.r[to.raw] = src_id;
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
      check_dest_castling_rights();
    };

    const auto double_push = [&] {
      new_pos.m_piece_list_sq[color].m[src_id] = to;
      new_pos.m_board.m[from.raw] = Place::empty;
      new_pos.m_board.m[to.raw] = src_place;
      new_pos.m_id.r[to.raw] = src_id;
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
      new_pos.m_enpassant = Square{narrow_cast<u8>((from.raw + to.raw) >> 1)};
    };

    const auto enpassant = [&] {
      const Square victim{narrow_cast<u8>((from.raw & 0x38) | (to.raw & 7))};
      const u8 victim_id = m_id.r[victim.raw];
      new_pos.m_piece_list_sq[color].m[src_id] = to;
      new_pos.m_piece_list_sq[!color].m[victim_id] = Square::invalid();
      new_pos.m_piece_list_ptype[!color].m[victim_id] = PieceType::none;
      new_pos.m_board.m[from.raw] = Place::empty;
      new_pos.m_board.m[victim.raw] = Place::empty;
      new_pos.m_board.m[to.raw] = src_place;
      new_pos.m_id.r[from.raw] = 0x80;
      new_pos.m_id.r[victim.raw] = 0x80;
      new_pos.m_id.r[to.raw] = src_id;
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
    };

    const auto castle = [&](u8 king_dest_file, u8 rook_dest_file) {
      const Square king_dest{narrow_cast<u8>((from.raw & 0x38) | king_dest_file)};
      const Square rook_dest{narrow_cast<u8>((from.raw & 0x38) | rook_dest_file)};
      const Square king_src = m.from();
      const Square rook_src = m.to();
      const u8 king_id = m_id.r[king_src.raw];
      const u8 rook_id = m_id.r[rook_src.raw];
      new_pos.m_piece_list_sq[color].m[king_id] = king_dest;
      new_pos.m_piece_list_sq[color].m[rook_id] = rook_dest;
      new_pos.m_board.m[king_src.raw] = Place::empty;
      new_pos.m_board.m[rook_src.raw] = Place::empty;
      new_pos.m_board.m[king_dest.raw] = Place::fromColorAndPtype(m_active_color, PieceType::k);
      new_pos.m_board.m[rook_dest.raw] = Place::fromColorAndPtype(m_active_color, PieceType::r);
      new_pos.m_id.r[king_src.raw] = 0x80;
      new_pos.m_id.r[rook_src.raw] = 0x80;
      new_pos.m_id.r[king_dest.raw] = king_id;
      new_pos.m_id.r[rook_dest.raw] = rook_id;
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
      new_pos.m_rook_info[color].clear();
    };

#define MF(x) (static_cast<int>(MoveFlags::x) >> 12)
    switch (static_cast<int>(m.flags()) >> 12) {
    case MF(normal):
      normal();
      break;
    case MF(double_push):
      double_push();
      break;
    case MF(castle_aside):
      castle(2, 3);
      break;
    case MF(castle_hside):
      castle(6, 5);
      break;
    case MF(promo_q):
      promo(PieceType::q);
      break;
    case MF(promo_n):
      promo(PieceType::n);
      break;
    case MF(promo_r):
      promo(PieceType::r);
      break;
    case MF(promo_b):
      promo(PieceType::b);
      break;
    case MF(capture):
      capture();
      break;
    case MF(enpassant):
      enpassant();
      break;
    case MF(cap_promo_q):
      cap_promo(PieceType::q);
      break;
    case MF(cap_promo_n):
      cap_promo(PieceType::n);
      break;
    case MF(cap_promo_r):
      cap_promo(PieceType::r);
      break;
    case MF(cap_promo_b):
      cap_promo(PieceType::b);
      break;
    default:
      std::unreachable();
    }
#undef MF

    if (m_rook_info[color] != new_pos.m_rook_info[color]) {
      // TODO: m_hash castling change
    }

    new_pos.m_ply++;
    new_pos.m_active_color = m_active_color.invert();
    // TODO: m_hash side change

    new_pos.m_attack_table = calcAttacksSlow();

    return new_pos;
  }

  auto Position::calcAttacksSlow() const -> std::array<Wordboard, 2> {
    std::array<Wordboard, 2> result{};
    for (int i = 0; i < 64; i++) {
      const Square sq{static_cast<u8>(i)};
      const auto [white, black] = calcAttacksSlow(sq);
      result[0].r[i] = white;
      result[1].r[i] = black;
    }
    return result;
  }

  auto Position::calcAttacksSlow(Square sq) const -> std::array<u16, 2> {
    const auto [ray_coords, ray_valid] = geometry::superpieceRays(sq);
    const v512 ray_places = vec::permute8(ray_coords, m_board.z);

    const u64 occupied = ray_places.nonzero8();
    const u64 color = ray_places.msb8();

    const u64 visible = geometry::superpieceAttacks(occupied, ray_valid) & occupied;

    const u64 white_attackers = ~color & visible & (ray_places & geometry::superpieceAttackerMask(Color::black)).nonzero8();
    const u64 black_attackers = color & visible & (ray_places & geometry::superpieceAttackerMask(Color::white)).nonzero8();
    const int white_attackers_count = std::popcount(white_attackers);
    const int black_attackers_count = std::popcount(black_attackers);
    const v128 white_attackers_coord = vec::compress8(white_attackers, ray_coords).to128();
    const v128 black_attackers_coord = vec::compress8(black_attackers, ray_coords).to128();
    return {
        vec::findset8(white_attackers_coord, white_attackers_count, m_piece_list_sq[0].x),
        vec::findset8(black_attackers_coord, black_attackers_count, m_piece_list_sq[1].x),
    };
  }

  auto Position::prettyPrint() const -> void {
    std::print("   +------------------------+\n");
    for (i8 rank = 7; rank >= 0; rank--) {
      std::print(" {} |", static_cast<char>('1' + rank));
      for (u8 file = 0; file < 8; file++) {
        const Square sq = Square::fromFileAndRank(file, static_cast<u8>(rank));
        const Place place = m_board.m[sq.raw];
        std::print(" {} ", place);
      }
      std::print("|\n");
    }
    std::print("   +------------------------+\n");
    std::print("     a  b  c  d  e  f  g  h  \n");
  }

  auto Position::printAttackTable() const -> void {
    // Indexing occurs in this order:
    // +---------+
    // |0 1 2 3 4|
    // |5       6|
    // |7 . x . 8|
    // |9       a|
    // |b c d e f|
    // +---------+
    const auto get = [this](Square sq, Square attacker) -> std::tuple<bool, int, int> {
      const auto [sq_file, sq_rank] = sq.toFileAndRank();
      const auto [attacker_file, attacker_rank] = attacker.toFileAndRank();
      const int file_diff = static_cast<int>(attacker_file) - static_cast<int>(sq_file);
      const int rank_diff = static_cast<int>(attacker_rank) - static_cast<int>(sq_rank);
      if (file_diff < 0 && rank_diff > 0 && -file_diff == rank_diff)
        return {true, 0, rank_diff};
      if (file_diff == -1 && rank_diff == 2)
        return {true, 1, 1};
      if (file_diff == 0 && rank_diff > 0)
        return {true, 2, rank_diff};
      if (file_diff == 1 && rank_diff == 2)
        return {true, 3, 1};
      if (file_diff > 0 && rank_diff > 0 && file_diff == rank_diff)
        return {true, 4, rank_diff};
      if (file_diff == -2 && rank_diff == 1)
        return {true, 5, 1};
      if (file_diff == 2 && rank_diff == 1)
        return {true, 6, 1};
      if (rank_diff == 0 && file_diff < 0)
        return {true, 7, -file_diff};
      if (rank_diff == 0 && file_diff > 0)
        return {true, 8, file_diff};
      if (file_diff == -2 && rank_diff == -1)
        return {true, 9, 1};
      if (file_diff == 2 && rank_diff == -1)
        return {true, 10, 1};
      if (file_diff < 0 && rank_diff < 0 && file_diff == rank_diff)
        return {true, 11, -rank_diff};
      if (file_diff == -1 && rank_diff == -2)
        return {true, 12, 1};
      if (file_diff == 0 && rank_diff < 0)
        return {true, 13, -rank_diff};
      if (file_diff == 1 && rank_diff == -2)
        return {true, 14, 1};
      if (file_diff > 0 && rank_diff < 0 && file_diff == -rank_diff)
        return {true, 15, -rank_diff};
      return {false, 0, -1};
    };

    std::array<std::array<int, 16>, 64> result{};
    for (int sq_raw = 0; sq_raw < 64; sq_raw++) {
      const Square sq{static_cast<u8>(sq_raw)};
      for (int color : {0, 1}) {
        const u16 attackers = m_attack_table[color].r[sq.raw];
        for (int id = 0; id < 16; id++) {
          if ((attackers >> id) & 1) {
            const Square attacker = m_piece_list_sq[color].m[id];
            const auto [valid, index, distance] = get(sq, attacker);
            if (!valid || result[sq_raw][index] != 0) {
              std::print("error: invalid attack to {}: piece id {}:{} is at {}\n", sq, color, id, attacker);
              std::print("m_attack_table[{}].m[{}] = 0x{:04x}\n", color, sq, attackers);
              std::print("m_piece_list_sq[{}] = ", color);
              m_piece_list_sq[color].dump();
              continue;
            }
            result[sq_raw][index] = distance;
          }
        }
      }
    }

    std::print("   +---------+---------+---------+---------+---------+---------+---------+---------+\n");
    for (i8 rank = 7; rank >= 0; rank--) {
      for (int row : {0, 1, 2, 3, 4}) {
        std::print(" {} |", row == 2 ? static_cast<char>('1' + rank) : ' ');
        for (u8 file = 0; file < 8; file++) {
          const Square sq = Square::fromFileAndRank(file, static_cast<u8>(rank));
          const auto dirs = result[sq.raw];
          const auto r = [dirs](int index) { return " 123456789"[dirs[index]]; };
          const auto n = [dirs](int index) { return " n????????"[dirs[index]]; };
          switch (row) {
          case 0:
            std::print("{} {} {} {} {}|", r(0), n(1), r(2), n(3), r(4));
            break;
          case 1:
            std::print("{}       {}|", n(5), n(6));
            break;
          case 2:
            std::print("{}   {}   {}|", r(7), m_board.m[sq.raw], r(8));
            break;
          case 3:
            std::print("{}       {}|", n(9), n(10));
            break;
          case 4:
            std::print("{} {} {} {} {}|", r(11), n(12), r(13), n(14), r(15));
            break;
          }
        }
        std::print("\n");
      }
      std::print("   +---------+---------+---------+---------+---------+---------+---------+---------+\n");
    }
    std::print("        a         b         c         d         e         f         g         h     \n");
  }

  auto Position::parse(std::string_view board_str, std::string_view color_str, std::string_view castle_str, std::string_view enpassant_str,
                       std::string_view irreversible_clock_str, std::string_view ply_str) -> std::expected<Position, ParseError> {
    Position result{};

    // Parse board
    {
      usize place_index = 0, i = 0;
      std::array<usize, 2> id{{0, 0}};
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
          const usize color = p->color().toIndex();
          usize &current_id = id[color];
          if (current_id >= result.m_piece_list_sq[color].m.size())
            return std::unexpected(ParseError::too_many_pieces);
          result.m_board.m[sq.raw] = *p;
          result.m_piece_list_sq[color].m[current_id] = sq;
          result.m_piece_list_ptype[color].m[current_id] = p->ptype();
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
        result.m_active_color = Color::black;
        break;
      case 'w':
        result.m_active_color = Color::white;
        break;
      default:
        return std::unexpected(ParseError::invalid_char);
      }
    }

    // Parse castling rights
    // TODO: Support FRC
    if (castle_str != "-") {
      for (const char ch : castle_str) {
        const auto castle_rights = [&](Color color, u8 rook_file) -> std::optional<ParseError> {
          const Square rook_sq = Square::fromFileAndRank(rook_file, color.toBackRank());
          const Square king_sq = Square::fromFileAndRank(4, color.toBackRank());
          if (result.m_board.m[rook_sq.raw].color() != color || result.m_board.m[rook_sq.raw].ptype() != PieceType::r ||
              result.m_board.m[king_sq.raw].color() != color || result.m_board.m[king_sq.raw].ptype() != PieceType::k)
            return ParseError::invalid_board;
          if (rook_file == 7)
            result.m_rook_info[color.toIndex()].hside = rook_sq;
          if (rook_file == 0)
            result.m_rook_info[color.toIndex()].aside = rook_sq;
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
      result.m_enpassant = enpassant.value();
    }

    // Parse irreversible clock
    if (const usize irreversible_clock = std::stoi(std::string{irreversible_clock_str}); irreversible_clock <= 200) {
      result.m_irreversible_clock = irreversible_clock;
    } else {
      return std::unexpected(ParseError::out_of_range);
    }

    // Parse ply
    if (const usize ply = std::stoi(std::string{ply_str}); ply != 0 && ply < 10000) {
      result.m_ply = (ply - 1) * 2 + static_cast<u16>(result.m_active_color.toIndex());
    } else {
      return std::unexpected(ParseError::out_of_range);
    }

    result.m_attack_table = result.calcAttacksSlow();

    return result;
  }

} // namespace rose
