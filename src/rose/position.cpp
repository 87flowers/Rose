#include "rose/position.hpp"

#include "rose/bitboard.hpp"
#include "rose/board.hpp"
#include "rose/common.hpp"
#include "rose/geometry.hpp"
#include "rose/move.hpp"
#include "rose/square.hpp"
#include "rose/util/string.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace rose {

  auto Position::add_attacks(Color color, Square sq, PieceId id, PieceType ptype) -> void {
    const auto [ray_coords, ray_valid] = geometry::superpiece_rays(sq);
    const u8x64 ray_places = ray_coords.swizzle(m_board.to_vector());
    const u8x64 iperm = geometry::superpiece_inverse_rays(sq);

    const m8x64 raymask = geometry::superpiece_attacks(ray_places, ray_valid);

    const m8x64 attacker_mask = raymask & geometry::attack_mask(color, ptype);
    const m8x64 add_mask = iperm.swizzle(attacker_mask).andnot(iperm.msb());

    m_attack_table[color.to_index()].raw |= add_mask.convert<u16>().mask(u16x64::splat(id.to_piece_mask().raw));
  }

  auto Position::remove_attacks(Color color, PieceId id) -> void {
    m_attack_table[color.to_index()].raw &= u16x64::splat(~id.to_piece_mask().raw);
  }

  auto Position::toggle_sliders_single(Square sq) -> void {
    const auto [ray_coords, ray_valid] = geometry::superpiece_rays(sq);
    const u8x64 ray_places = ray_coords.swizzle(m_board.to_vector());
    const u8x64 iperm = geometry::superpiece_inverse_rays_flipped(sq);

    const m8x64 sliders = geometry::sliders_from_rays(ray_places);
    const m8x64 raymask = geometry::superpiece_attacks(ray_places, ray_valid);
    const m8x64 visible_sliders = raymask & sliders;

    u8x64 slider_ids = geometry::slider_broadcast(visible_sliders.mask(ray_places));
    slider_ids = geometry::flip_mask(raymask).mask(slider_ids);
    slider_ids = iperm.swizzle(slider_ids);

    const m16x64 valid_ids = slider_ids.nonzeros().convert<u16>();
    const m16x64 color = slider_ids.msb().convert<u16>();

    const u16x64 bits = valid_ids.mask(u16x64::splat(1) << (slider_ids & u8x64::splat(0xF)).convert<u16>());

    m_attack_table[0].raw ^= (~color).mask(bits);
    m_attack_table[1].raw ^= color.mask(bits);
  }

  template<bool update_sliders>
  auto Position::add_piece(Color color, Square sq, PieceId id, PieceType ptype) -> void {
    if constexpr (update_sliders)
      toggle_sliders_single(sq);
    add_attacks(color, sq, id, ptype);

    m_piece_list_sq[color.to_index()][id] = sq;
    m_piece_list_ptype[color.to_index()][id] = ptype;
    m_board[sq] = Place::make(color, ptype, id);
  }

  template<bool update_sliders>
  auto Position::remove_piece(Color color, Square sq, PieceId id) -> void {
    if constexpr (update_sliders)
      toggle_sliders_single(sq);
    remove_attacks(color, id);

    m_piece_list_sq[color.to_index()][id] = Square::invalid();
    m_piece_list_ptype[color.to_index()][id] = PieceType::none;
    m_board[sq] = Place::empty;
  }

  auto Position::move_piece(Color color, Square src_sq, Square dst_sq, PieceId id, PieceType src_ptype, PieceType dst_ptype) -> void {
    const auto [src_ray_coords, src_ray_valid] = geometry::superpiece_rays(src_sq);
    const auto [dst_ray_coords, dst_ray_valid] = geometry::superpiece_rays(dst_sq);

    u8x64 board = m_board.to_vector();
    const u8x64 src_ray_places = src_ray_coords.swizzle(board);
    board = m8x64 {~src_sq.to_bitboard().raw}.mask(board);
    const u8x64 dst_ray_places = dst_ray_coords.swizzle(board);
    board = m8x64 {dst_sq.to_bitboard().raw}.select(board, u8x64::splat(Place::make(color, dst_ptype, id).raw));
    std::memcpy(m_board.mailbox.data(), &board, sizeof(board));

    const u8x64 src_iperm = geometry::superpiece_inverse_rays_flipped(src_sq);
    const u8x64 dst_iperm = geometry::superpiece_inverse_rays_flipped(dst_sq);

    const m8x64 src_sliders = geometry::sliders_from_rays(src_ray_places);
    const m8x64 dst_sliders = geometry::sliders_from_rays(dst_ray_places);
    const m8x64 src_raymask = geometry::superpiece_attacks(src_ray_places, src_ray_valid);
    const m8x64 dst_raymask = geometry::superpiece_attacks(dst_ray_places, dst_ray_valid);
    const m8x64 src_visible_sliders = src_raymask & src_sliders;
    const m8x64 dst_visible_sliders = dst_raymask & dst_sliders;

    u8x64 src_slider_ids = geometry::slider_broadcast(src_visible_sliders.mask(src_ray_places));
    u8x64 dst_slider_ids = geometry::slider_broadcast(dst_visible_sliders.mask(dst_ray_places));
    src_slider_ids = geometry::flip_mask(src_raymask).mask(src_slider_ids);
    dst_slider_ids = geometry::flip_mask(dst_raymask).mask(dst_slider_ids);
    src_slider_ids = src_iperm.swizzle(src_slider_ids);
    dst_slider_ids = dst_iperm.swizzle(dst_slider_ids);

    const m16x64 src_color = src_slider_ids.msb().convert<u16>();
    const m16x64 dst_color = dst_slider_ids.msb().convert<u16>();

    const m16x64 src_valid_ids = src_slider_ids.nonzeros().convert<u16>();
    const m16x64 dst_valid_ids = dst_slider_ids.nonzeros().convert<u16>();
    const u16x64 src_bits = src_valid_ids.mask(u16x64::splat(1) << (src_slider_ids & u8x64::splat(0xF)).convert<u16>());
    const u16x64 dst_bits = dst_valid_ids.mask(u16x64::splat(1) << (dst_slider_ids & u8x64::splat(0xF)).convert<u16>());

    const u16x64 id_bit = u16x64::splat(id.to_piece_mask().raw);
    const m8x64 dst_attack_mask = dst_raymask & geometry::attack_mask(color, dst_ptype);
    const m16x64 add_mask = dst_iperm.swizzle(geometry::flip_mask(dst_attack_mask)).convert<u16>();

    m_attack_table[0].raw ^= (~src_color).mask(src_bits) ^ (~dst_color).mask(dst_bits);
    m_attack_table[1].raw ^= src_color.mask(src_bits) ^ dst_color.mask(dst_bits);

    switch (color.raw) {
    case Color::white:
      m_attack_table[0].raw = m_attack_table[0].raw.andnot(id_bit) | add_mask.mask(id_bit);
      m_piece_list_sq[0][id] = dst_sq;
      m_piece_list_ptype[0][id] = dst_ptype;
      break;
    case Color::black:
      m_attack_table[1].raw = m_attack_table[1].raw.andnot(id_bit) | add_mask.mask(id_bit);
      m_piece_list_sq[1][id] = dst_sq;
      m_piece_list_ptype[1][id] = dst_ptype;
      break;
    }
  }

  auto Position::mutate_piece(Square sq, PieceId src_id, Color dst_color, PieceId dst_id, PieceType dst_ptype) -> void {
    const auto [ray_coords, ray_valid] = geometry::superpiece_rays(sq);

    u8x64 board = m_board.to_vector();
    const u8x64 ray_places = ray_coords.swizzle(board);
    board = m8x64 {sq.to_bitboard().raw}.select(board, u8x64::splat(Place::make(dst_color, dst_ptype, dst_id).raw));
    std::memcpy(m_board.mailbox.data(), &board, sizeof(board));

    const u8x64 iperm = geometry::superpiece_inverse_rays(sq);

    const m8x64 raymask = geometry::superpiece_attacks(ray_places, ray_valid);

    const m8x64 attacker_mask = raymask & geometry::attack_mask(dst_color, dst_ptype);
    const m8x64 add_mask = iperm.swizzle(attacker_mask).andnot(iperm.msb());

    switch (dst_color.raw) {
    case Color::white:
      m_attack_table[1].raw &= u16x64::splat(~src_id.to_piece_mask().raw);
      m_attack_table[0].raw |= add_mask.convert<u16>().mask(u16x64::splat(dst_id.to_piece_mask().raw));
      m_piece_list_sq[1][src_id] = Square::invalid();
      m_piece_list_ptype[1][src_id] = PieceType::none;
      m_piece_list_sq[0][dst_id] = sq;
      m_piece_list_ptype[0][dst_id] = dst_ptype;
      break;
    case Color::black:
      m_attack_table[0].raw &= u16x64::splat(~src_id.to_piece_mask().raw);
      m_attack_table[1].raw |= add_mask.convert<u16>().mask(u16x64::splat(dst_id.to_piece_mask().raw));
      m_piece_list_sq[0][src_id] = Square::invalid();
      m_piece_list_ptype[0][src_id] = PieceType::none;
      m_piece_list_sq[1][dst_id] = sq;
      m_piece_list_ptype[1][dst_id] = dst_ptype;
      break;
    }
  }

  auto Position::startpos() -> Position {
    static const Position startpos = Position::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").value();
    return startpos;
  }

  auto Position::move(Move m) const -> Position {
    Position new_pos = *this;

    const Square from = m.from();
    const Square to = m.to();
    const Place src_place = m_board[from];
    const Place dest_place = m_board[to];
    const PieceId src_id = src_place.id();
    const PieceId dest_id = dest_place.id();

    if (new_pos.m_enpassant.is_valid()) {
      new_pos.m_enpassant = Square::invalid();
    }

    const auto check_src_castling_rights = [&] {
      new_pos.m_rook_info.unset(m_stm, from);
      if (src_place.ptype() == PieceType::k) {
        new_pos.m_rook_info.clear(m_stm);
      }
    };

    const auto check_dest_castling_rights = [&] {
      new_pos.m_rook_info.unset(!m_stm, to);
    };

    const auto normal = [&] {
      new_pos.move_piece(m_stm, from, to, src_id, src_place.ptype(), src_place.ptype());
      if (src_place.ptype() != PieceType::p) {
        new_pos.m_50mr++;
      } else {
        new_pos.m_50mr = 0;
      }
      check_src_castling_rights();
    };

    const auto cap_normal = [&] {
      new_pos.remove_piece<true>(m_stm, from, src_id);
      new_pos.mutate_piece(to, dest_id, m_stm, src_id, src_place.ptype());
      new_pos.m_50mr = 0;
      check_src_castling_rights();
      check_dest_castling_rights();
    };

    const auto promo = [&](auto ptype) {
      new_pos.move_piece(m_stm, from, to, src_id, src_place.ptype(), decltype(ptype)::value);
      new_pos.m_50mr = 0;
    };

    const auto cap_promo = [&](auto ptype) {
      new_pos.remove_piece<true>(m_stm, from, src_id);
      new_pos.mutate_piece(to, dest_id, m_stm, src_id, decltype(ptype)::value);
      new_pos.m_50mr = 0;
      check_dest_castling_rights();
    };

    const auto double_push = [&] {
      new_pos.move_piece(m_stm, from, to, src_id, src_place.ptype(), src_place.ptype());
      new_pos.m_50mr = 0;
      new_pos.m_enpassant = Square {narrow_cast<u8>((from.raw + to.raw) >> 1)};
    };

    const auto enpassant = [&] {
      const Square victim = Square::from_file_and_rank(to.file(), from.rank());
      const PieceId victim_id = m_board[victim].id();

      new_pos.move_piece(m_stm, from, to, src_id, src_place.ptype(), src_place.ptype());
      new_pos.remove_piece<true>(!m_stm, victim, victim_id);

      new_pos.m_50mr = 0;
      check_src_castling_rights();
      check_dest_castling_rights();
    };

    const auto castle = [&](u8 king_dest_file, u8 rook_dest_file) {
      const Square king_dest {narrow_cast<u8>((from.raw & 0x38) | king_dest_file)};
      const Square rook_dest {narrow_cast<u8>((from.raw & 0x38) | rook_dest_file)};
      const Square king_src = m.from();
      const Square rook_src = m.to();
      const PieceId king_id = m_board[king_src].id();
      const PieceId rook_id = m_board[rook_src].id();

      new_pos.remove_piece<true>(m_stm, king_src, king_id);
      new_pos.remove_piece<true>(m_stm, rook_src, rook_id);
      new_pos.add_piece<true>(m_stm, king_dest, king_id, PieceType::k);
      new_pos.add_piece<true>(m_stm, rook_dest, rook_id, PieceType::r);

      new_pos.m_50mr++;
      new_pos.m_rook_info.clear(m_stm);
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
      promo(std::integral_constant<PieceType, PieceType::q> {});
      break;
    case MF(promo_n):
      promo(std::integral_constant<PieceType, PieceType::n> {});
      break;
    case MF(promo_r):
      promo(std::integral_constant<PieceType, PieceType::r> {});
      break;
    case MF(promo_b):
      promo(std::integral_constant<PieceType, PieceType::b> {});
      break;
    case MF(cap_normal):
      cap_normal();
      break;
    case MF(enpassant):
      enpassant();
      break;
    case MF(cap_promo_q):
      cap_promo(std::integral_constant<PieceType, PieceType::q> {});
      break;
    case MF(cap_promo_n):
      cap_promo(std::integral_constant<PieceType, PieceType::n> {});
      break;
    case MF(cap_promo_r):
      cap_promo(std::integral_constant<PieceType, PieceType::r> {});
      break;
    case MF(cap_promo_b):
      cap_promo(std::integral_constant<PieceType, PieceType::b> {});
      break;
    }
#undef MF

    new_pos.m_ply++;
    new_pos.m_stm = !m_stm;

    return new_pos;
  }

  auto Position::calc_pin_info() const -> std::tuple<std::array<PieceMask, 64>, Bitboard> {
    const Square sq = king_sq(m_stm);

    const auto [ray_coords, ray_valid_premask] = geometry::superpiece_rays(sq);
    const m8x64 ray_valid = ray_valid_premask & m8x64 {0xFEFEFEFEFEFEFEFE};
    const u8x64 ray_places = ray_coords.swizzle(m_board.to_vector());
    const u8x64 iperm = geometry::superpiece_inverse_rays(sq);

    const m8x64 blockers = ray_places.nonzeros();
    const m8x64 color = ray_places.msb();
    const m8x64 enemy = (color ^ m8x64 {m_stm.to_bitboard().raw}) & blockers;

    // Closest blockers
    const m8x64 potentially_pinned = blockers & geometry::superpiece_attacks(ray_places, ray_valid);

    // Find all enemy sliders with the correct attacks for the rays they're on
    const m8x64 maybe_attacking = enemy & geometry::sliders_from_rays(ray_places);
    // Second closest blockers
    const m8x64 not_closest = blockers.andnot(potentially_pinned);
    const m8x64 pin_raymasks = geometry::superpiece_attacks(not_closest.to_vector().convert<u8>(), ray_valid);
    const m8x64 potential_attackers = not_closest & pin_raymasks;
    // Second closest blockers that are of the correct type to be pinning attackers.
    const m8x64 attackers = maybe_attacking & potential_attackers;

    // A closest blocker is pinned if it has a valid pinning attacker.
#if LPS_AVX512
    const m8x64 no_pinner_mask {std::bit_cast<vm8x64>(std::bit_cast<u64x8>(attackers.to_vector()).zeros().to_vector()).to_bits()};
#else
    const m8x64 no_pinner_mask = std::bit_cast<m8x64>(std::bit_cast<m64x8>(attackers).to_vector().zeros());
#endif
    const m8x64 pinned = potentially_pinned.andnot(enemy).andnot(no_pinner_mask);

    // Translate to valid move rays
    const u8x64 pinned_ids = pin_raymasks.mask(geometry::lane_broadcast(pinned.mask(ray_places)));
    const u8x64 board_layout = iperm.swizzle(pinned_ids);

    // Convert from list of squares to piecemask
    const int pinned_count = pinned.popcount();
    const u8x16 pinned_coord = pinned.compress(ray_coords).extract_aligned<u8x16, 0>();
    const PieceMask piece_mask = geometry::find_set(pinned_coord, pinned_count, piece_list_sq(m_stm).to_vector());

    // Generate attack table mask
    const u16x64 ones = u16x64::splat(1);
    const m16x64 valid_ids = board_layout.nonzeros().convert<u16>();
    const u16x64 masked_ids = (board_layout & u8x64::splat(0xF)).convert<u16>();
    const u16x64 bits = valid_ids.mask(ones << masked_ids);
    const u16x64 at_mask = u16x64::splat(~piece_mask.raw) | bits;

    const Bitboard pinned_bb {board_layout.nonzeros().to_bits()};

    return {Wordboard {m_attack_table[m_stm.to_index()].raw & at_mask}.to_mailbox(), pinned_bb};
  }

  auto Position::calc_attacks_slow() const -> std::array<Wordboard, 2> {
    std::array<std::array<PieceMask, 64>, 2> result {};
    for (int i = 0; i < 64; i++) {
      const Square sq {static_cast<u8>(i)};
      const auto [white, black] = calc_attacks_slow(sq);
      result[0][i] = white;
      result[1][i] = black;
    }
    return std::bit_cast<std::array<Wordboard, 2>>(result);
  }

  auto Position::calc_attacks_slow(Square sq) const -> std::array<PieceMask, 2> {
    const auto [ray_coords, ray_valid] = geometry::superpiece_rays(sq);
    const u8x64 ray_places = ray_coords.swizzle(m_board.to_vector());

    const m8x64 occupied = ray_places.nonzeros();
    const m8x64 color = ray_places.msb();

    const m8x64 visible = geometry::superpiece_attacks(ray_places, ray_valid) & occupied;

    const m8x64 attackers = geometry::attackers_from_rays(ray_places);
    const m8x64 white_attackers = ~color & visible & attackers;
    const m8x64 black_attackers = color & visible & attackers;

    const int white_attackers_count = white_attackers.popcount();
    const int black_attackers_count = black_attackers.popcount();
    const u8x16 white_attackers_coord = white_attackers.compress(ray_coords).extract_aligned<u8x16, 0>();
    const u8x16 black_attackers_coord = black_attackers.compress(ray_coords).extract_aligned<u8x16, 0>();
    return {
      geometry::find_set(white_attackers_coord, white_attackers_count, m_piece_list_sq[0].to_vector()),
      geometry::find_set(black_attackers_coord, black_attackers_count, m_piece_list_sq[1].to_vector()),
    };
  }

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
          result.m_board[sq] = Place::make(static_cast<Color::Underlying>(color), PieceType::k, current_id);
          result.m_piece_list_sq[color].m[current_id] = sq;
          result.m_piece_list_ptype[color].m[current_id] = PieceType::k;
          place_index++;
        } else if (const auto p = PieceType::parse(ch); p) {
          const auto [pt, c] = *p;
          const usize color = c.to_index();
          u8& current_id = id[color];
          if (current_id >= result.m_piece_list_sq[color].m.size())
            return std::unexpected(ParseError::too_many_pieces);
          result.m_board[sq] = Place::make(c, pt, current_id);
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
            if (rook_place.is_empty()) {
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

    result.m_attack_table = result.calc_attacks_slow();
    // result.m_hash = result.calcHashSlow();

    return result;
  }

  auto Position::dump() const -> void {
    fmt::print("fen: {}\n", *this);
    // m_attack_table:
    for (int c : {0, 1}) {
      fmt::print("m_attack_table[{}]:\n", c);
      for (int r = 7; r >= 0; r--) {
        for (int f = 0; f < 8; f++) {
          const Square sq = Square::from_file_and_rank(f, r);
          fmt::print("{:04x} ", m_attack_table[c].read(sq).raw);
        }
        fmt::print("\n");
      }
    }
    // m_board:
    {
      fmt::print("m_board:\n");
      for (int r = 7; r >= 0; r--) {
        for (int f = 0; f < 8; f++) {
          const Square sq = Square::from_file_and_rank(f, r);
          fmt::print("{:02x}:{} ", (u8)m_board[sq].raw, m_board[sq]);
        }
        fmt::print("\n");
      }
    }
    // m_piece_list:
    for (int c : {0, 1}) {
      fmt::print("m_piece_list[{}]: ", c);
      for (u8 id = 0; id < 16; id++) {
        fmt::print("{}:{} ", m_piece_list_ptype[c][PieceId {id}], m_piece_list_sq[c][PieceId {id}]);
      }
      fmt::print("\n");
    }
    fmt::print("m_rook_info: {:08x}\n", m_rook_info.raw);
    fmt::print("m_50mr: {}\n", m_50mr);
    fmt::print("m_ply: {}\n", m_ply);
    fmt::print("m_enpassant: {}\n", m_enpassant);
    fmt::print("m_stm: {}\n", m_stm);
    // calc_pin_info:
    {
      const auto [at, pinned] = calc_pin_info();
      fmt::print("pinned-masked m_attack_table:\n");
      for (int r = 7; r >= 0; r--) {
        for (int f = 0; f < 8; f++) {
          const Square sq = Square::from_file_and_rank(f, r);
          fmt::print("{:04x} ", at[sq.raw].raw);
        }
        fmt::print("\n");
      }
      fmt::print("pinned:\n");
      for (int r = 7; r >= 0; r--) {
        for (int f = 0; f < 8; f++) {
          const Square sq = Square::from_file_and_rank(f, r);
          fmt::print("{} ", pinned.read(sq) ? 1 : 0);
        }
        fmt::print("\n");
      }
    }
  }

}  // namespace rose
