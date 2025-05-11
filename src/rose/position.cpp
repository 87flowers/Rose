#include "rose/position.h"

#include <bit>
#include <print>
#include <type_traits>
#include <utility>

#include "rose/hash.h"

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

  auto Position::startpos() -> Position {
    static const Position startpos = Position::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").value();
    return startpos;
  }

  auto Position::move(Move m) const -> Position {
    Position new_pos = *this;

    const Square from = m.from();
    const Square to = m.to();
    const Place src_place = m_board.m[from.raw];
    const Place dest_place = m_board.m[to.raw];
    const u8 src_id = src_place.id();
    const u8 dest_id = dest_place.id();
    const bool color = m_active_color.toIndex();

    if (m_enpassant.isValid()) {
      new_pos.m_hash ^= hash::enpassant_table[new_pos.m_enpassant.file()];
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
        new_pos.m_rook_info[!color].unset(to);
      }
    };

    const auto normal = [&] {
      new_pos.movePiece(color, from, to, src_id, src_place.ptype());
      new_pos.m_hash ^= hash::movePiece(from, to, src_place);
      if (src_place.ptype() != PieceType::p) {
        new_pos.m_50mr++;
      } else {
        new_pos.m_50mr = 0;
      }
      check_src_castling_rights();
    };

    const auto capture = [&] {
      new_pos.m_piece_list_sq[!color].m[dest_id] = Square::invalid();
      new_pos.m_piece_list_ptype[!color].m[dest_id] = PieceType::none;
      new_pos.removeAttacks(!color, dest_id);
      new_pos.movePiece<false>(color, from, to, src_id, src_place.ptype());
      new_pos.m_hash ^= hash::removePiece(to, dest_place);
      new_pos.m_hash ^= hash::movePiece(from, to, src_place);
      new_pos.m_50mr = 0;
      check_src_castling_rights();
      check_dest_castling_rights();
    };

    const auto promo = [&](auto ptype) {
      new_pos.m_piece_list_ptype[color].m[src_id] = ptype;
      new_pos.movePiece<true, decltype(ptype)::value>(color, from, to, src_id, src_place.ptype());
      new_pos.m_hash ^= hash::promo(from, to, m_active_color, decltype(ptype)::value);
      new_pos.m_50mr = 0;
    };

    const auto cap_promo = [&](auto ptype) {
      new_pos.m_piece_list_sq[!color].m[dest_id] = Square::invalid();
      new_pos.m_piece_list_ptype[color].m[src_id] = ptype;
      new_pos.m_piece_list_ptype[!color].m[dest_id] = PieceType::none;
      new_pos.removeAttacks(!color, dest_id);
      new_pos.movePiece<false, decltype(ptype)::value>(color, from, to, src_id, src_place.ptype());
      new_pos.m_hash ^= hash::removePiece(to, dest_place);
      new_pos.m_hash ^= hash::promo(from, to, m_active_color, decltype(ptype)::value);
      new_pos.m_50mr = 0;
      check_dest_castling_rights();
    };

    const auto double_push = [&] {
      new_pos.movePiece(color, from, to, src_id, src_place.ptype());
      new_pos.m_hash ^= hash::movePiece(from, to, src_place);
      new_pos.m_50mr = 0;
      new_pos.m_enpassant = Square{narrow_cast<u8>((from.raw + to.raw) >> 1)};
      new_pos.m_hash ^= hash::enpassant_table[new_pos.m_enpassant.file()];
    };

    const auto enpassant = [&] {
      const Square victim{narrow_cast<u8>((from.raw & 0x38) | (to.raw & 7))};
      const u8 victim_id = m_board.m[victim.raw].id();
      new_pos.movePiece(color, from, to, src_id, src_place.ptype());
      new_pos.m_hash ^= hash::movePiece(from, to, src_place);

      new_pos.incrementalSliderUpdate(victim);
      new_pos.m_piece_list_sq[!color].m[victim_id] = Square::invalid();
      new_pos.m_piece_list_ptype[!color].m[victim_id] = PieceType::none;
      new_pos.m_board.m[victim.raw] = Place::empty;
      new_pos.removeAttacks(!color, victim_id);
      new_pos.m_hash ^= hash::removePiece(victim, m_active_color.invert(), PieceType::p);

      new_pos.m_50mr = 0;
      check_src_castling_rights();
      check_dest_castling_rights();
    };

    const auto castle = [&](u8 king_dest_file, u8 rook_dest_file) {
      const Square king_dest{narrow_cast<u8>((from.raw & 0x38) | king_dest_file)};
      const Square rook_dest{narrow_cast<u8>((from.raw & 0x38) | rook_dest_file)};
      const Square king_src = m.from();
      const Square rook_src = m.to();
      const u8 king_id = m_board.m[king_src.raw].id();
      const u8 rook_id = m_board.m[rook_src.raw].id();

      new_pos.incrementalSliderUpdate(king_src);
      new_pos.m_board.m[king_src.raw] = Place::empty;
      new_pos.removeAttacks(color, king_id);
      new_pos.incrementalSliderUpdate(rook_src);
      new_pos.m_board.m[rook_src.raw] = Place::empty;
      new_pos.removeAttacks(color, rook_id);
      new_pos.m_board.m[king_dest.raw] = Place::fromColorAndPtypeAndId(m_active_color, PieceType::k, king_id);
      new_pos.incrementalSliderUpdate(king_dest);
      new_pos.addAttacks(color, king_dest, king_id, PieceType::k);
      new_pos.m_board.m[rook_dest.raw] = Place::fromColorAndPtypeAndId(m_active_color, PieceType::r, rook_id);
      new_pos.incrementalSliderUpdate(rook_dest);
      new_pos.addAttacks(color, rook_dest, rook_id, PieceType::r);

      new_pos.m_piece_list_sq[color].m[king_id] = king_dest;
      new_pos.m_piece_list_sq[color].m[rook_id] = rook_dest;

      new_pos.m_hash ^= hash::movePiece(king_src, king_dest, m_active_color, PieceType::k);
      new_pos.m_hash ^= hash::movePiece(rook_src, rook_dest, m_active_color, PieceType::r);

      new_pos.m_50mr++;
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
      promo(std::integral_constant<PieceType, PieceType::q>{});
      break;
    case MF(promo_n):
      promo(std::integral_constant<PieceType, PieceType::n>{});
      break;
    case MF(promo_r):
      promo(std::integral_constant<PieceType, PieceType::r>{});
      break;
    case MF(promo_b):
      promo(std::integral_constant<PieceType, PieceType::b>{});
      break;
    case MF(capture):
      capture();
      break;
    case MF(enpassant):
      enpassant();
      break;
    case MF(cap_promo_q):
      cap_promo(std::integral_constant<PieceType, PieceType::q>{});
      break;
    case MF(cap_promo_n):
      cap_promo(std::integral_constant<PieceType, PieceType::n>{});
      break;
    case MF(cap_promo_r):
      cap_promo(std::integral_constant<PieceType, PieceType::r>{});
      break;
    case MF(cap_promo_b):
      cap_promo(std::integral_constant<PieceType, PieceType::b>{});
      break;
    }
#undef MF

    if (m_rook_info != new_pos.m_rook_info) {
      if (m_rook_info[0].aside != new_pos.m_rook_info[0].aside)
        new_pos.m_hash ^= hash::castle_table[0][0];
      if (m_rook_info[0].hside != new_pos.m_rook_info[0].hside)
        new_pos.m_hash ^= hash::castle_table[0][1];
      if (m_rook_info[1].aside != new_pos.m_rook_info[1].aside)
        new_pos.m_hash ^= hash::castle_table[1][0];
      if (m_rook_info[1].hside != new_pos.m_rook_info[1].hside)
        new_pos.m_hash ^= hash::castle_table[1][1];
    }

    new_pos.m_ply++;
    new_pos.m_active_color = m_active_color.invert();
    new_pos.m_hash ^= hash::move;

    rose_assert(new_pos.m_hash == new_pos.calcHashSlow(), "{} [{:016x}] : {} : {} [{:016x} {:016x}]", *this, m_hash, m, new_pos, new_pos.m_hash,
                new_pos.calcHashSlow());
    rose_assert(new_pos.m_attack_table == new_pos.calcAttacksSlow());

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

    const u64 attackers = geometry::attackersFromRays(ray_places);
    const u64 white_attackers = ~color & visible & attackers;
    const u64 black_attackers = color & visible & attackers;

    const int white_attackers_count = std::popcount(white_attackers);
    const int black_attackers_count = std::popcount(black_attackers);
    const v128 white_attackers_coord = vec::compress8(white_attackers, ray_coords).to128();
    const v128 black_attackers_coord = vec::compress8(black_attackers, ray_coords).to128();
    return {
        vec::findset8(white_attackers_coord, white_attackers_count, m_piece_list_sq[0].x),
        vec::findset8(black_attackers_coord, black_attackers_count, m_piece_list_sq[1].x),
    };
  }

  auto Position::calcHashSlow() const -> u64 {
    u64 result = 0;
    for (int sq = 0; sq < 64; sq++) {
      const u8 p = m_board.r[sq];
      result ^= hash::piece_table[p >> 4][sq];
    }
    if (m_enpassant.isValid())
      result ^= hash::enpassant_table[m_enpassant.file()];
    if (m_rook_info[0].aside.isValid())
      result ^= hash::castle_table[0][0];
    if (m_rook_info[0].hside.isValid())
      result ^= hash::castle_table[0][1];
    if (m_rook_info[1].aside.isValid())
      result ^= hash::castle_table[1][0];
    if (m_rook_info[1].hside.isValid())
      result ^= hash::castle_table[1][1];
    if (m_active_color == Color::black)
      result ^= hash::move;
    return result;
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
                       std::string_view clock_50mr_str, std::string_view ply_str) -> std::expected<Position, ParseError> {
    Position result{};

    // Parse board
    {
      usize place_index = 0, i = 0;
      std::array<u8, 2> id{{0x01, 0x01}};
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
        } else if (ch == 'k' || ch == 'K') {
          const usize color = ch == 'k';
          const u8 current_id = 0x00;
          if (result.m_piece_list_sq[color].m[current_id].isValid())
            return std::unexpected(ParseError::too_many_kings);
          result.m_board.m[sq.raw] = Place::fromColorAndPtypeAndId(static_cast<Color::Inner>(color), PieceType::k, current_id);
          result.m_piece_list_sq[color].m[current_id] = sq;
          result.m_piece_list_ptype[color].m[current_id] = PieceType::k;
          place_index++;
        } else if (const auto p = PieceType::parse(ch); p) {
          const auto [pt, c] = *p;
          const usize color = c.toIndex();
          u8 &current_id = id[color];
          if (current_id >= result.m_piece_list_sq[color].m.size())
            return std::unexpected(ParseError::too_many_pieces);
          result.m_board.m[sq.raw] = Place::fromColorAndPtypeAndId(c, pt, current_id);
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
          const Square king_sq = result.kingSq(color);
          const Place rook_place = result.m_board.m[rook_sq.raw];
          if (rook_place.color() != color || rook_place.ptype() != PieceType::r || king_sq.rank() != color.toBackRank())
            return ParseError::invalid_board;
          if (rook_file < king_sq.file())
            result.m_rook_info[color.toIndex()].aside = rook_sq;
          if (rook_file > king_sq.file())
            result.m_rook_info[color.toIndex()].hside = rook_sq;
          return std::nullopt;
        };
        const auto scan_for_rook = [&](Color color, int file, int direction) -> std::optional<ParseError> {
          while (true) {
            if (file < 0 || file > 7)
              return ParseError::invalid_board;
            const Square rook_sq = Square::fromFileAndRank(static_cast<u8>(file), color.toBackRank());
            const Place rook_place = result.m_board.m[rook_sq.raw];
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
    if (const usize clock_50mr = std::stoi(std::string{clock_50mr_str}); clock_50mr <= 200) {
      result.m_50mr = clock_50mr;
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
    result.m_hash = result.calcHashSlow();

    return result;
  }

  template <bool update_dst_silders, PieceType dst_ptype>
  forceinline auto Position::movePiece(bool color, Square src, Square dst, u8 id, PieceType ptype) -> void {
    m_piece_list_sq[color].m[id] = dst;

    const auto [src_ray_coords, src_ray_valid] = geometry::superpieceRays(src);
    const auto [dst_ray_coords, dst_ray_valid] = geometry::superpieceRays(dst);

    v512 board = m_board.z;
    const v512 new_dst_place = dst_ptype == PieceType::none ? vec::permute8(v512::broadcast8(src.raw), board)
                                                            : v512::broadcast8(Place::fromColorAndPtypeAndId(Color::Inner{color}, dst_ptype, id).raw);
    const v512 src_ray_places = vec::permute8(src_ray_coords, board);
    board = vec::mask8(~(static_cast<u64>(1) << src.raw), board);
    const v512 dst_ray_places = vec::permute8(dst_ray_coords, board);
    board = vec::blend8(static_cast<u64>(1) << dst.raw, board, new_dst_place);
    m_board.z = board;

    const v512 src_swapped_perm = geometry::superpieceInverseRaysSwapped(src);
    const v512 dst_swapped_perm = geometry::superpieceInverseRaysSwapped(dst);

    const u64 src_blockers = src_ray_places.nonzero8();
    const u64 dst_blockers = dst_ray_places.nonzero8();
    const u64 src_sliders = geometry::slidersFromRays(src_ray_places);
    const u64 dst_sliders = geometry::slidersFromRays(dst_ray_places);

    const u64 src_raymask = geometry::superpieceAttacks(src_blockers, src_ray_valid);
    const u64 dst_raymask = geometry::superpieceAttacks(dst_blockers, dst_ray_valid);

    const u64 src_visible_sliders = src_raymask & src_sliders;
    const u64 dst_visible_sliders = dst_raymask & dst_sliders;

    // Broadcasts slider id to its 8-lane group
    const v512 src_visible_sliders_ids = vec::lanebroadcast8to64(vec::mask8(src_visible_sliders, vec::permute8(src_ray_coords, board)));
    const v512 dst_visible_sliders_ids = vec::lanebroadcast8to64(vec::mask8(dst_visible_sliders, dst_ray_places));
    // Squares to update
    const v512 src_ids_to_update = vec::mask8(std::rotl(src_raymask & geometry::non_horse_attack_mask, 32), src_visible_sliders_ids);
    const v512 dst_ids_to_update = vec::mask8(std::rotl(dst_raymask & geometry::non_horse_attack_mask, 32), dst_visible_sliders_ids);

    // Permute from rays back into a board
    const v512 src_ids_in_board_layout = vec::permute8_mz(~src_swapped_perm.msb8(), src_swapped_perm, src_ids_to_update);
    const v512 dst_ids_in_board_layout = vec::permute8_mz(~dst_swapped_perm.msb8(), dst_swapped_perm, dst_ids_to_update);
    // We use zero as an invalid id beacuse it is guaranteed to be a king which is never a slider.
    const u64 src_valid_ids = src_ids_in_board_layout.nonzero8();
    const u64 dst_valid_ids = dst_ids_in_board_layout.nonzero8();
    const u64 src_color = src_ids_in_board_layout.msb8();
    const u64 dst_color = dst_ids_in_board_layout.msb8();

    const v512 src_masked_ids = src_ids_in_board_layout & v512::broadcast8(0xF);
    const v512 dst_masked_ids = dst_ids_in_board_layout & v512::broadcast8(0xF);

    const v512 ones = v512::broadcast16(1);
    const v512 src_bits0 = vec::shl16_mz(static_cast<u32>(src_valid_ids), ones, vec::zext8to16(src_masked_ids.to256()));
    const v512 src_bits1 = vec::shl16_mz(static_cast<u32>(src_valid_ids >> 32), ones, vec::zext8to16(vec::extract256<1>(src_masked_ids)));
    const v512 dst_bits0 = vec::shl16_mz(static_cast<u32>(dst_valid_ids), ones, vec::zext8to16(dst_masked_ids.to256()));
    const v512 dst_bits1 = vec::shl16_mz(static_cast<u32>(dst_valid_ids >> 32), ones, vec::zext8to16(vec::extract256<1>(dst_masked_ids)));

    const v512 rm_mask = v512::broadcast16(~narrow_cast<u16>(1 << (id & 0xF)));
    const v512 add_bit = v512::broadcast16(narrow_cast<u16>(1 << (id & 0xF)));
    const u64 attacker_mask = dst_raymask & geometry::attackMask(Color::Inner{color}, dst_ptype == PieceType::none ? ptype.raw : dst_ptype.raw);
    const u64 add_mask = vec::bitshuffle_m(~dst_swapped_perm.msb8(), v512::broadcast64(std::rotl(attacker_mask, 32)), dst_swapped_perm);

    v512 at00 = vec::mask16(~static_cast<u32>(src_color), src_bits0);
    v512 at01 = vec::mask16(~static_cast<u32>(src_color >> 32), src_bits1);
    v512 at10 = vec::mask16(static_cast<u32>(src_color), src_bits0);
    v512 at11 = vec::mask16(static_cast<u32>(src_color >> 32), src_bits1);
    if constexpr (update_dst_silders) {
      at00 ^= vec::mask16(~static_cast<u32>(dst_color), dst_bits0);
      at01 ^= vec::mask16(~static_cast<u32>(dst_color >> 32), dst_bits1);
      at10 ^= vec::mask16(static_cast<u32>(dst_color), dst_bits0);
      at11 ^= vec::mask16(static_cast<u32>(dst_color >> 32), dst_bits1);
    }
    m_attack_table[0].z[0] ^= at00;
    m_attack_table[0].z[1] ^= at01;
    m_attack_table[1].z[0] ^= at10;
    m_attack_table[1].z[1] ^= at11;

    m_attack_table[color].z[0] = (m_attack_table[color].z[0] & rm_mask) | vec::mask16(static_cast<u32>(add_mask), add_bit);
    m_attack_table[color].z[1] = (m_attack_table[color].z[1] & rm_mask) | vec::mask16(static_cast<u32>(add_mask >> 32), add_bit);
  }

  forceinline auto Position::incrementalSliderUpdate(Square sq) -> void {
    const auto [ray_coords, ray_valid] = geometry::superpieceRays(sq);
    const v512 ray_places = vec::permute8(ray_coords, m_board.z);

    const v512 swapped_perm = geometry::superpieceInverseRaysSwapped(sq);

    const u64 blockers = ray_places.nonzero8();
    const u64 sliders = (ray_places & v512::broadcast8(Place::slider_bit)).nonzero8() & (ray_places & geometry::sliderMask).nonzero8();

    const u64 raymask = geometry::superpieceAttacks(blockers, ray_valid) & geometry::non_horse_attack_mask;
    const u64 visible_sliders = raymask & sliders;

    // Broadcasts slider id to its 8-lane group
    const v512 visible_sliders_ids = vec::lanebroadcast8to64(vec::mask8(visible_sliders, ray_places));
    // Squares to update
    const v512 ids_to_update = vec::mask8(std::rotl(raymask, 32), visible_sliders_ids);

    // Permute from rays back into a board
    const v512 ids_in_board_layout = vec::permute8_mz(~swapped_perm.msb8(), swapped_perm, ids_to_update);
    // We use zero as an invalid id beacuse it is guaranteed to be a king which is never a slider.
    const u64 valid_ids = ids_in_board_layout.nonzero8();
    const u64 color = ids_in_board_layout.msb8();

    const v512 masked_ids = ids_in_board_layout & v512::broadcast8(0xF);

    const v512 ones = v512::broadcast16(1);
    const v512 bits0 = vec::shl16_mz(static_cast<u32>(valid_ids), ones, vec::zext8to16(masked_ids.to256()));
    const v512 bits1 = vec::shl16_mz(static_cast<u32>(valid_ids >> 32), ones, vec::zext8to16(vec::extract256<1>(masked_ids)));

    m_attack_table[0].z[0] ^= vec::mask16(~static_cast<u32>(color), bits0);
    m_attack_table[0].z[1] ^= vec::mask16(~static_cast<u32>(color >> 32), bits1);
    m_attack_table[1].z[0] ^= vec::mask16(static_cast<u32>(color), bits0);
    m_attack_table[1].z[1] ^= vec::mask16(static_cast<u32>(color >> 32), bits1);
  }

  forceinline auto Position::removeAttacks(bool color, u8 id) -> void {
    const v512 mask = v512::broadcast16(~narrow_cast<u16>(1 << (id & 0xF)));
    m_attack_table[color].z[0] &= mask;
    m_attack_table[color].z[1] &= mask;
  }

  forceinline auto Position::addAttacks(bool color, Square sq, u8 id, PieceType ptype) -> void {
    const v512 bit = v512::broadcast16(narrow_cast<u16>(1 << (id & 0xF)));

    const auto [ray_coords, ray_valid] = geometry::superpieceRays(sq);
    const v512 ray_places = vec::permute8(ray_coords, m_board.z);
    const v512 inverse_ray_permutation = geometry::superpieceInverseRays(sq);

    const u64 blockers = ray_places.nonzero8();
    const u64 raymask = geometry::superpieceAttacks(blockers, ray_valid);

    const u64 attacker_mask = raymask & geometry::attackMask(Color::Inner{color}, ptype.raw);
    const u64 add_mask = vec::bitshuffle_m(~inverse_ray_permutation.msb8(), v512::broadcast64(attacker_mask), inverse_ray_permutation);

    m_attack_table[color].z[0] = m_attack_table[color].z[0] | vec::mask16(static_cast<u32>(add_mask), bit);
    m_attack_table[color].z[1] = m_attack_table[color].z[1] | vec::mask16(static_cast<u32>(add_mask >> 32), bit);
  }

} // namespace rose
