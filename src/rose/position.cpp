#include "rose/position.h"

#include <bit>
#include <print>
#include <type_traits>
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
        new_pos.m_rook_info[!color].unset(to);
      }
    };

    const auto normal = [&] {
      new_pos.movePiece(color, from, to, src_id, src_place.ptype());
      // TODO: m_hash
      if (src_place.ptype() != PieceType::p) {
        new_pos.m_irreversible_clock++;
      } else {
        new_pos.m_irreversible_clock = 0;
      }
      check_src_castling_rights();
    };

    const auto capture = [&] {
      new_pos.m_piece_list_sq[!color].m[dest_id & 0xF] = Square::invalid();
      new_pos.m_piece_list_ptype[!color].m[dest_id & 0xF] = PieceType::none;
      new_pos.removeAttacks(!color, dest_id);
      new_pos.movePiece<false>(color, from, to, src_id, src_place.ptype());
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
      check_src_castling_rights();
      check_dest_castling_rights();
    };

    const auto promo = [&](auto ptype) {
      new_pos.m_piece_list_ptype[color].m[src_id & 0xF] = ptype;
      new_pos.movePiece<true, decltype(ptype)::value>(color, from, to, src_id, src_place.ptype());
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
    };

    const auto cap_promo = [&](auto ptype) {
      new_pos.m_piece_list_sq[!color].m[dest_id & 0xF] = Square::invalid();
      new_pos.m_piece_list_ptype[color].m[src_id & 0xF] = ptype;
      new_pos.m_piece_list_ptype[!color].m[dest_id & 0xF] = PieceType::none;
      new_pos.removeAttacks(!color, dest_id);
      new_pos.movePiece<false, decltype(ptype)::value>(color, from, to, src_id, src_place.ptype());
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
      check_dest_castling_rights();
    };

    const auto double_push = [&] {
      new_pos.movePiece(color, from, to, src_id, src_place.ptype());
      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
      new_pos.m_enpassant = Square{narrow_cast<u8>((from.raw + to.raw) >> 1)};
    };

    const auto enpassant = [&] {
      const Square victim{narrow_cast<u8>((from.raw & 0x38) | (to.raw & 7))};
      const u8 victim_id = m_id.r[victim.raw];
      new_pos.movePiece(color, from, to, src_id, src_place.ptype());

      new_pos.incrementalSliderUpdate(victim);
      new_pos.m_piece_list_sq[!color].m[victim_id & 0xF] = Square::invalid();
      new_pos.m_piece_list_ptype[!color].m[victim_id & 0xF] = PieceType::none;
      new_pos.m_board.m[victim.raw] = Place::empty;
      new_pos.m_id.r[victim.raw] = 0xFF;

      // TODO: m_hash
      new_pos.m_irreversible_clock = 0;
      new_pos.removeAttacks(!color, victim_id);
      check_src_castling_rights();
      check_dest_castling_rights();
    };

    const auto castle = [&](u8 king_dest_file, u8 rook_dest_file) {
      const Square king_dest{narrow_cast<u8>((from.raw & 0x38) | king_dest_file)};
      const Square rook_dest{narrow_cast<u8>((from.raw & 0x38) | rook_dest_file)};
      const Square king_src = m.from();
      const Square rook_src = m.to();
      const u8 king_id = m_id.r[king_src.raw];
      const u8 rook_id = m_id.r[rook_src.raw];

      new_pos.movePiece(color, king_src, king_dest, king_id, PieceType::k);
      new_pos.movePiece(color, rook_src, rook_dest, rook_id, PieceType::r);

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
      std::array<usize, 2> id{{0x01, 0x81}};
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
          const u8 current_id = ch == 'k' ? 0x80 : 0x00;
          if (result.m_piece_list_sq[color].m[current_id & 0xF].isValid())
            return std::unexpected(ParseError::too_many_kings);
          result.m_board.m[sq.raw] = Place::fromColorAndPtype(static_cast<Color::Inner>(color), PieceType::k);
          result.m_id.r[sq.raw] = current_id;
          result.m_piece_list_sq[color].m[current_id & 0xF] = sq;
          result.m_piece_list_ptype[color].m[current_id & 0xF] = PieceType::k;
          place_index++;
        } else if (const auto p = Place::parse(ch); p) {
          const usize color = p->color().toIndex();
          usize &current_id = id[color];
          if (current_id & 0xF >= result.m_piece_list_sq[color].m.size())
            return std::unexpected(ParseError::too_many_pieces);
          result.m_board.m[sq.raw] = *p;
          result.m_id.r[sq.raw] = current_id;
          result.m_piece_list_sq[color].m[current_id & 0xF] = sq;
          result.m_piece_list_ptype[color].m[current_id & 0xF] = p->ptype();
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

  template <bool update_to_silders, PieceType dest_ptype>
  forceinline auto Position::movePiece(bool color, Square from, Square to, u8 id, PieceType ptype) -> void {
    m_piece_list_sq[color].m[id & 0xF] = to;
    m_id.r[to.raw] = m_id.r[from.raw];

    const auto [from_ray_coords, from_ray_valid] = geometry::superpieceRays(from);
    const auto [to_ray_coords, to_ray_valid] = geometry::superpieceRays(to);

    v512 board = m_board.z;
    const v512 src_place = dest_ptype == PieceType::none ? vec::permute8(v512::broadcast8(from.raw), board)
                                                         : v512::broadcast8(Place::fromColorAndPtype(Color::Inner{color}, dest_ptype).raw);
    const v512 from_ray_places = vec::permute8(from_ray_coords, board);
    board = vec::mask8(~(static_cast<u64>(1) << from.raw), board);
    const v512 to_ray_places = vec::permute8(to_ray_coords, board);
    m_board.z = vec::blend8(static_cast<u64>(1) << to.raw, board, src_place);

    const v512 from_ray_ids = vec::permute8(from_ray_coords, m_id.z);
    const v512 to_ray_ids = vec::permute8(to_ray_coords, m_id.z);

    const v512 from_swapped_perm = geometry::superpieceInverseRaysSwapped(from);
    const v512 to_swapped_perm = geometry::superpieceInverseRaysSwapped(to);

    const u64 from_blockers = from_ray_places.nonzero8();
    const u64 to_blockers = to_ray_places.nonzero8();
    const u64 from_sliders = (from_ray_places & geometry::superpieceSliderMask).nonzero8();
    const u64 to_sliders = (to_ray_places & geometry::superpieceSliderMask).nonzero8();

    const u64 from_raymask = geometry::superpieceAttacks(from_blockers, from_ray_valid) & geometry::non_horse_attack_mask;
    const u64 to_raymask_with_horses = geometry::superpieceAttacks(to_blockers, to_ray_valid);
    const u64 to_raymask = to_raymask_with_horses & geometry::non_horse_attack_mask;

    const u64 from_visible_sliders = from_raymask & from_sliders;
    const u64 to_visible_sliders = to_raymask & to_sliders;

    // Broadcasts slider id to its 8-lane group
    const v512 from_visible_sliders_ids = vec::gf2p8matmul8(
        v512::broadcast8(0xFF), vec::gf2p8matmul8(v512::broadcast64(0x0102040810204080), vec::mask8(from_visible_sliders, from_ray_ids)));
    const v512 to_visible_sliders_ids = vec::gf2p8matmul8(
        v512::broadcast8(0xFF), vec::gf2p8matmul8(v512::broadcast64(0x0102040810204080), vec::mask8(to_visible_sliders, to_ray_ids)));
    // Squares to update
    const v512 from_ids_to_update = vec::mask8(std::rotl(from_raymask, 32), from_visible_sliders_ids);
    const v512 to_ids_to_update = vec::mask8(std::rotl(to_raymask, 32), to_visible_sliders_ids);

    // Permute from rays back into a board
    const v512 from_ids_in_board_layout = vec::permute8_mz(~from_swapped_perm.msb8(), from_swapped_perm, from_ids_to_update);
    const v512 to_ids_in_board_layout = vec::permute8_mz(~to_swapped_perm.msb8(), to_swapped_perm, to_ids_to_update);
    // We use zero as an invalid id beacuse it is guaranteed to be a king which is never a slider.
    const u64 from_valid_ids = from_ids_in_board_layout.nonzero8();
    const u64 to_valid_ids = to_ids_in_board_layout.nonzero8();
    const u64 from_color = from_ids_in_board_layout.msb8();
    const u64 to_color = to_ids_in_board_layout.msb8();

    const v512 from_masked_ids = from_ids_in_board_layout & v512::broadcast8(0xF);
    const v512 to_masked_ids = to_ids_in_board_layout & v512::broadcast8(0xF);

    const v512 ones = v512::broadcast16(1);
    const v512 from_bits0 = vec::shl16_mz(static_cast<u32>(from_valid_ids), ones, vec::zext8to16(from_masked_ids.to256()));
    const v512 from_bits1 = vec::shl16_mz(static_cast<u32>(from_valid_ids >> 32), ones, vec::zext8to16(vec::extract256<1>(from_masked_ids)));
    const v512 to_bits0 = vec::shl16_mz(static_cast<u32>(to_valid_ids), ones, vec::zext8to16(to_masked_ids.to256()));
    const v512 to_bits1 = vec::shl16_mz(static_cast<u32>(to_valid_ids >> 32), ones, vec::zext8to16(vec::extract256<1>(to_masked_ids)));

    m_attack_table[0].z[0] = m_attack_table[0].z[0] ^ vec::mask16(~static_cast<u32>(from_color), from_bits0);
    if constexpr (update_to_silders)
      m_attack_table[0].z[0] = m_attack_table[0].z[0] ^ vec::mask16(~static_cast<u32>(to_color), to_bits0);
    m_attack_table[0].z[1] = m_attack_table[0].z[1] ^ vec::mask16(~static_cast<u32>(from_color >> 32), from_bits1);
    if constexpr (update_to_silders)
      m_attack_table[0].z[1] = m_attack_table[0].z[1] ^ vec::mask16(~static_cast<u32>(to_color >> 32), to_bits1);
    m_attack_table[1].z[0] = m_attack_table[1].z[0] ^ vec::mask16(static_cast<u32>(from_color), from_bits0);
    if constexpr (update_to_silders)
      m_attack_table[1].z[0] = m_attack_table[1].z[0] ^ vec::mask16(static_cast<u32>(to_color), to_bits0);
    m_attack_table[1].z[1] = m_attack_table[1].z[1] ^ vec::mask16(static_cast<u32>(from_color >> 32), from_bits1);
    if constexpr (update_to_silders)
      m_attack_table[1].z[1] = m_attack_table[1].z[1] ^ vec::mask16(static_cast<u32>(to_color >> 32), to_bits1);

    const v512 rm_mask = v512::broadcast16(~narrow_cast<u16>(1 << (id & 0xF)));
    m_attack_table[color].z[0] = m_attack_table[color].z[0] & rm_mask;
    m_attack_table[color].z[1] = m_attack_table[color].z[1] & rm_mask;

    const v512 add_bit = v512::broadcast16(narrow_cast<u16>(1 << (id & 0xF)));
    const v512 attacker_mask = vec::mask8(to_raymask_with_horses, geometry::superpieceAttackerMask(static_cast<Color::Inner>(color))) &
                               v512::broadcast8(dest_ptype == PieceType::none ? ptype.raw : dest_ptype.raw);
    const u64 add_mask = vec::permute8_mz(~to_swapped_perm.msb8(), to_swapped_perm, vec::shuffle128<0b01001110>(attacker_mask)).nonzero8();

    m_attack_table[color].z[0] = m_attack_table[color].z[0] | vec::mask16(static_cast<u32>(add_mask), add_bit);
    m_attack_table[color].z[1] = m_attack_table[color].z[1] | vec::mask16(static_cast<u32>(add_mask >> 32), add_bit);
  }

  forceinline auto Position::incrementalSliderUpdate(Square sq) -> void {
    const auto [ray_coords, ray_valid] = geometry::superpieceRays(sq);
    const v512 ray_places = vec::permute8(ray_coords, m_board.z);
    const v512 masked_ray_places = ray_places & geometry::superpieceSliderMask;
    const v512 ray_ids = vec::permute8(ray_coords, m_id.z);

    const v512 swapped_perm = geometry::superpieceInverseRaysSwapped(sq);

    const u64 blockers = ray_places.nonzero8();
    const u64 sliders = masked_ray_places.nonzero8();

    const u64 raymask = geometry::superpieceAttacks(blockers, ray_valid) & geometry::non_horse_attack_mask;
    const u64 visible_sliders = raymask & sliders;

    // Broadcasts slider id to its 8-lane group
    const v512 visible_sliders_ids =
        vec::gf2p8matmul8(v512::broadcast8(0xFF), vec::gf2p8matmul8(v512::broadcast64(0x0102040810204080), vec::mask8(visible_sliders, ray_ids)));
    // Squares to update
    const v512 ids_to_update = vec::mask8((raymask << 32) | (raymask >> 32), visible_sliders_ids);

    // Permute from rays back into a board
    const v512 ids_in_board_layout = vec::permute8_mz(~swapped_perm.msb8(), swapped_perm, ids_to_update);
    // We use zero as an invalid id beacuse it is guaranteed to be a king which is never a slider.
    const u64 valid_ids = ids_in_board_layout.nonzero8();
    const u64 color = ids_in_board_layout.msb8();

    const v512 masked_ids = ids_in_board_layout & v512::broadcast8(0xF);

    const v512 ones = v512::broadcast16(1);
    const v512 bits0 = vec::shl16_mz(static_cast<u32>(valid_ids), ones, vec::zext8to16(masked_ids.to256()));
    const v512 bits1 = vec::shl16_mz(static_cast<u32>(valid_ids >> 32), ones, vec::zext8to16(vec::extract256<1>(masked_ids)));

    m_attack_table[0].z[0] = m_attack_table[0].z[0] ^ vec::mask16(~static_cast<u32>(color), bits0);
    m_attack_table[0].z[1] = m_attack_table[0].z[1] ^ vec::mask16(~static_cast<u32>(color >> 32), bits1);
    m_attack_table[1].z[0] = m_attack_table[1].z[0] ^ vec::mask16(static_cast<u32>(color), bits0);
    m_attack_table[1].z[1] = m_attack_table[1].z[1] ^ vec::mask16(static_cast<u32>(color >> 32), bits1);
  }

  forceinline auto Position::removeAttacks(bool color, u8 id) -> void {
    const v512 mask = v512::broadcast16(~narrow_cast<u16>(1 << (id & 0xF)));
    rose_assert(color == (id >> 7));
    m_attack_table[color].z[0] = m_attack_table[color].z[0] & mask;
    m_attack_table[color].z[1] = m_attack_table[color].z[1] & mask;
  }

  forceinline auto Position::addAttacks(bool color, Square sq, u8 id, PieceType ptype) -> void {
    const v512 bit = v512::broadcast16(narrow_cast<u16>(1 << (id & 0xF)));
    rose_assert(color == (id >> 7));

    const auto [ray_coords, ray_valid] = geometry::superpieceRays(sq);
    const v512 ray_places = vec::permute8(ray_coords, m_board.z);
    const v512 inverse_ray_permutation = geometry::superpieceInverseRays(sq);

    const u64 blockers = ray_places.nonzero8();
    const u64 raymask = geometry::superpieceAttacks(blockers, ray_valid);

    const v512 attacker_mask = vec::mask8(raymask, geometry::superpieceAttackerMask(static_cast<Color::Inner>(color))) & v512::broadcast8(ptype.raw);
    const u64 mask = vec::permute8_mz(~inverse_ray_permutation.msb8(), inverse_ray_permutation, attacker_mask).nonzero8();

    m_attack_table[color].z[0] = m_attack_table[color].z[0] | vec::mask16(static_cast<u32>(mask), bit);
    m_attack_table[color].z[1] = m_attack_table[color].z[1] | vec::mask16(static_cast<u32>(mask >> 32), bit);
  }

} // namespace rose
