#include "rose/position.hpp"

#include "rose/bitboard.hpp"
#include "rose/board.hpp"
#include "rose/common.hpp"
#include "rose/eval/nnue/arch.hpp"
#include "rose/geometry.hpp"
#include "rose/move.hpp"
#include "rose/movegen.hpp"
#include "rose/rays.hpp"
#include "rose/score.hpp"
#include "rose/square.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/string.hpp"

#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <string_view>
#include <utility>
#include <vector>

namespace rose {

  static inline auto backrank_ray(Square a, Square b) -> Bitboard {
    return a.raw > b.raw ? Bitboard {(a.to_bitboard().raw << 1) - b.to_bitboard().raw} : Bitboard {(b.to_bitboard().raw << 1) - a.to_bitboard().raw};
  }

  static inline auto is_castle_legal(const Position& pos, Square rook, i8 rook_dest, i8 king_dest) -> bool {
    const Color stm = pos.stm();
    const Square king = pos.king_sq(stm);

    const Bitboard empty = pos.board().empty_bitboard();
    const Bitboard danger = pos.attack_table(!stm).bitboard_any();
    const auto [_, pinned] = pos.pin_info();

    const Bitboard king_bb = king.to_bitboard();
    const Bitboard rook_bb = rook.to_bitboard();
    const Bitboard rook_ray = backrank_ray(rook, Square::from_file_and_rank(rook_dest, king.rank()));
    const Bitboard king_ray = backrank_ray(king, Square::from_file_and_rank(king_dest, king.rank()));

    const Bitboard clear = empty | king_bb | rook_bb;

    rose_assert(pos.board()[rook].ptype() == PieceType::r && pos.board()[rook].color() == stm);
    return ((~clear & rook_ray).is_empty() && ((~clear | danger) & king_ray).is_empty() && (rook_bb & pinned).is_empty());
  }

#if !defined(LPS_AVX512) || !LPS_AVX512
  template<class T, class U>
  static inline auto concat(U a, U b) -> T {
    return std::bit_cast<T>(std::array {a, b});
  }
#endif

  static inline auto expand_mask(m8x64 x) -> m16x64 {
#if LPS_AVX512
    return x.convert<u16>();
#else
    return concat<m16x64>(x.to_vector().zip_low(x.to_vector()), x.to_vector().zip_high(x.to_vector()));
#endif
  }

  static inline auto id_to_at(u8x64 ids) -> u16x64 {
    ids &= u8x64::splat(0xF);
#if LPS_AVX512
    const m16x64 valid_ids = ids.nonzeros().convert<u16>();
    return valid_ids.mask(u16x64::splat(1) << ids.convert<u16>());
#else
    constexpr u8x16 lo_shift {{0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    constexpr u8x16 hi_shift {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}};
    const u8x64 bits_lo = ids.swizzle(lo_shift);
    const u8x64 bits_hi = ids.swizzle(hi_shift);
    return concat<u16x64>(bits_lo.zip_low(bits_hi), bits_lo.zip_high(bits_hi));
#endif
  }

  template<eval::concepts::Observer Observer>
  auto Position::add_attacks(Observer observer, Color color, Square sq, PieceId id, PieceType ptype) -> void {
    const auto [ray_coords, ray_valid] = geometry::superpiece_rays(sq);
    const u8x64 ray_places = ray_coords.swizzle(m_board.to_vector());
    const u8x64 iperm = geometry::superpiece_inverse_rays(sq);
    const m8x64 raymask = geometry::superpiece_attacks(ray_places, ray_valid);
    const m8x64 attacker_mask = raymask & geometry::attack_mask(color, ptype);

    observer.on_add_focus_threats(*this, ptype, sq, attacker_mask.to_bits(), ray_coords, ray_places);

    const m8x64 add_mask = iperm.swizzle(attacker_mask).andnot(iperm.msb());
    m_attack_table[color.to_index()].raw |= expand_mask(add_mask).mask(u16x64::splat(id.to_piece_mask().raw));
  }

  template<eval::concepts::Observer Observer>
  auto Position::remove_attacks(Observer observer, Color color, Square sq, PieceId id, PieceType ptype) -> void {
    observer.on_remove_focus_threats(*this, ptype, sq);

    m_attack_table[color.to_index()].raw &= u16x64::splat(~id.to_piece_mask().raw);
  }

  template<bool piece_removed, eval::concepts::Observer Observer>
  auto Position::toggle_sliders_single(Observer observer, Square sq) -> void {
    const auto [ray_coords, ray_valid] = geometry::superpiece_rays(sq);
    const u8x64 ray_places = ray_coords.swizzle(m_board.to_vector());
    const u8x64 iperm = geometry::superpiece_inverse_rays_flipped(sq);

    const m8x64 sliders = geometry::sliders_from_rays(ray_places);
    const m8x64 raymask = geometry::superpiece_attacks(ray_places, ray_valid);
    const m8x64 visible_sliders = raymask & sliders;

    if constexpr (piece_removed) {
      observer.on_add_discovered_threats(*this, sliders, raymask, ray_coords, ray_places);
    } else {
      observer.on_remove_discovered_threats(*this, sliders, raymask, ray_coords, ray_places);
    }

    u8x64 slider_ids = geometry::slider_broadcast(visible_sliders.mask(ray_places));
    slider_ids = geometry::flip_mask(raymask).mask(slider_ids);
    slider_ids = iperm.swizzle(slider_ids);

    const m16x64 color = slider_ids.msb().convert<u16>();

    const u16x64 bits = id_to_at(slider_ids);

    m_attack_table[0].raw ^= (~color).mask(bits);
    m_attack_table[1].raw ^= color.mask(bits);
  }

  template<bool update_sliders, eval::concepts::Observer Observer>
  auto Position::add_piece(Observer observer, Color color, Square sq, PieceId id, PieceType ptype) -> void {
    if constexpr (update_sliders)
      toggle_sliders_single<false>(observer, sq);
    add_attacks(observer, color, sq, id, ptype);

    m_piece_list_sq[color.to_index()][id] = sq;
    m_piece_list_ptype[color.to_index()][id] = ptype;

    u8x64 board = m_board.to_vector();
    board = m8x64 {sq.to_bitboard().raw}.select(board, u8x64::splat(Place::make(color, ptype, id).raw));
    std::memcpy(m_board.mailbox.data(), &board, sizeof(board));
  }

  template<bool update_sliders, eval::concepts::Observer Observer>
  auto Position::remove_piece(Observer observer, Color color, Square sq, PieceId id, PieceType ptype) -> void {
    if constexpr (update_sliders)
      toggle_sliders_single<true>(observer, sq);
    remove_attacks(observer, color, sq, id, ptype);

    m_piece_list_sq[color.to_index()][id] = Square::invalid();
    m_piece_list_ptype[color.to_index()][id] = PieceType::none;
    if constexpr (update_sliders) {
      u8x64 board = m_board.to_vector();
      board = m8x64 {~sq.to_bitboard().raw}.mask(board);
      std::memcpy(m_board.mailbox.data(), &board, sizeof(board));
    }
  }

  template<bool is_capture, eval::concepts::Observer Observer>
  auto Position::move_piece(Observer observer, Color color, Square src_sq, Square dst_sq, PieceId id, PieceType src_ptype, PieceType dst_ptype)
    -> void {
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

    u8x64 src_slider_ids = geometry::slider_broadcast(src_visible_sliders.mask(is_capture ? src_ray_coords.swizzle(board) : src_ray_places));
    u8x64 dst_slider_ids = geometry::slider_broadcast(dst_visible_sliders.mask(dst_ray_places));
    src_slider_ids = geometry::flip_mask(src_raymask).mask(src_slider_ids);
    dst_slider_ids = geometry::flip_mask(dst_raymask).mask(dst_slider_ids);
    src_slider_ids = src_iperm.swizzle(src_slider_ids);
    dst_slider_ids = dst_iperm.swizzle(dst_slider_ids);

    const m16x64 src_color = expand_mask(src_slider_ids.msb());
    const m16x64 dst_color = expand_mask(dst_slider_ids.msb());

    const u16x64 src_bits = id_to_at(src_slider_ids);
    const u16x64 dst_bits = id_to_at(dst_slider_ids);

    const u16x64 id_bit = u16x64::splat(id.to_piece_mask().raw);
    const m8x64 dst_attack_mask = dst_raymask & geometry::attack_mask(color, dst_ptype);
    const m16x64 add_mask = expand_mask((~dst_iperm.msb() & dst_iperm.swizzle(geometry::flip_mask(dst_attack_mask))));

    if constexpr (is_capture) {
      m_attack_table[0].raw ^= (~src_color).mask(src_bits);
      m_attack_table[1].raw ^= src_color.mask(src_bits);
    } else {
      m_attack_table[0].raw ^= (~src_color).mask(src_bits) ^ (~dst_color).mask(dst_bits);
      m_attack_table[1].raw ^= src_color.mask(src_bits) ^ dst_color.mask(dst_bits);
    }

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

  auto Position::startpos() -> Position {
    static const Position startpos = Position::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").value();
    return startpos;
  }

  static auto frc_backrank(usize index) -> std::array<PieceType, 8> {
    rose_assert(index < 960);
    std::array<PieceType, 8> rank {};

    const usize b1 = index % 4;
    index /= 4;
    const usize b2 = index % 4;
    index /= 4;
    const usize q = index % 6;
    index /= 6;

    constexpr std::array<std::array<usize, 2>, 10> knight_lut {{
      {{0, 0}},
      {{0, 1}},
      {{0, 2}},
      {{0, 3}},
      {{1, 1}},
      {{1, 2}},
      {{1, 3}},
      {{2, 2}},
      {{2, 3}},
      {{3, 3}},
    }};
    const auto [n1, n2] = knight_lut[index];

    // Insert ptype into place-th empty space
    const auto insert = [&](PieceType ptype, usize place) {
      for (usize i = 0, j = 0;; i++) {
        if (rank[i] == PieceType::none) {
          if (j == place) {
            rank[i] = ptype;
            break;
          }
          j++;
        }
      }
    };

    rank[b1 * 2 + 1] = PieceType::b;
    rank[b2 * 2 + 0] = PieceType::b;
    insert(PieceType::q, q);
    insert(PieceType::n, n1);
    insert(PieceType::n, n2);
    insert(PieceType::r, 0);
    insert(PieceType::k, 0);
    insert(PieceType::r, 0);

    return rank;
  }

  static auto rank_to_string(Color color, std::array<PieceType, 8> rank) -> std::string {
    std::string str;
    for (const PieceType ptype : rank)
      str += ptype.to_char(color);
    return str;
  }

  auto Position::frcstartpos(usize index) -> Position {
    const auto rank = frc_backrank(index);
    const auto black_rank = rank_to_string(Color::black, rank);
    const auto white_rank = rank_to_string(Color::white, rank);
    return Position::parse(black_rank + "/pppppppp/8/8/8/8/PPPPPPPP/" + white_rank + " w KQkq - 0 1").value();
  }

  auto Position::dfrcstartpos(usize index) -> Position {
    const auto black_rank = rank_to_string(Color::black, frc_backrank(index / 960));
    const auto white_rank = rank_to_string(Color::white, frc_backrank(index % 960));
    return Position::parse(black_rank + "/pppppppp/8/8/8/8/PPPPPPPP/" + white_rank + " w KQkq - 0 1").value();
  }

  auto Position::is_castle_aside_legal() const -> bool {
    return is_castle_legal(*this, m_rook_info.aside(m_stm), 3, 2);
  }

  auto Position::is_castle_hside_legal() const -> bool {
    return is_castle_legal(*this, m_rook_info.hside(m_stm), 5, 6);
  }

  auto Position::is_legal(Move m) const -> bool {
    if (m.is_none()) {
      return false;
    }

    const Square king = king_sq(m_stm);

    const Place src = m_board[m.from()];
    const Place dst = m_board[m.to()];

    const auto [at, pinned] = pin_info();

    const bool valid_attack = at[m.to().to_index()].is_set(src.id());

    if (src.is_empty() || src.color() != m_stm)
      return false;

    if (src.ptype() == PieceType::k) {
      if (m.flags() == MoveFlags::castle_aside && m.to() == m_rook_info.aside(m_stm) && is_castle_aside_legal())
        return true;
      if (m.flags() == MoveFlags::castle_hside && m.to() == m_rook_info.hside(m_stm) && is_castle_hside_legal())
        return true;

      const Bitboard danger = attack_table(!m_stm).bitboard_any();
      if (danger.read(m.to()))
        return false;
      for (const PieceId c : checkers())
        if (what_is(!m_stm, c).is_slider() && rays::king_invalid_destinations(king, where_is(!m_stm, c)).read(m.to()))
          return false;

      if (m.flags() == MoveFlags::cap_normal)
        return valid_attack && !dst.is_empty() && dst.color() != m_stm;
      if (m.flags() == MoveFlags::normal)
        return valid_attack && dst.is_empty();
      return false;
    }

    const PieceMask checker_ids = checkers();

    if (!checker_ids.is_empty()) {
      if (checker_ids.popcount() > 1)
        return false;

      const Square checker_sq = where_is(!m_stm, checker_ids.lsb());
      const PieceType checker_ptype = what_is(!m_stm, checker_ids.lsb());
      const Bitboard valid_destinations = checker_ptype == PieceType::n ? checker_sq.to_bitboard() : rays::calc_ray_to(king, checker_sq);

      if (m.is_enpassant() && checker_ptype != PieceType::p)
        return false;
      if (!m.is_enpassant() && !valid_destinations.read(m.to()))
        return false;
    }

    if (src.ptype() == PieceType::p) {
      if (m.is_castle())
        return false;
      if (m.is_promo() != (m.to().relative_rank(m_stm) == 7))
        return false;

      if (m.is_enpassant()) {
        if (m.from().rank() == king.rank()) {
          const Square ep_victim = Square::from_file_and_rank(m_enpassant.file(), m_stm == Color::white ? 4 : 3);
          const Bitboard potential_pinners =
            (m_board.bitboard_for<PieceType::r>(!m_stm) | m_board.bitboard_for<PieceType::q>(!m_stm)) & Bitboard::rank_mask(king.rank());
          const Bitboard occ = m_board.occupied_bitboard() ^ king.to_bitboard() ^ ep_victim.to_bitboard() ^ m.from().to_bitboard();
          for (const Square potential_pinner : potential_pinners)
            if ((rays::calc_ray_to(king, potential_pinner) & occ).popcount() == 1)
              return false;
        }
        return m.to() == m_enpassant && valid_attack;
      }

      const Bitboard pinned_quiet_pawns = pinned & ~Bitboard::file_mask(king.file());
      const i32 rank_delta = m.to().relative_rank(m_stm) - m.from().relative_rank(m_stm);
      const bool same_file = m.to().file() == m.from().file();

      if (m.is_double_push()) {
        const Square between {narrow_cast<u8>((m.from().raw + m.to().raw) >> 1)};
        return rank_delta == 2 && same_file && dst.is_empty() && m_board[between].is_empty() && m.from().relative_rank(m_stm) == 1 &&
               !pinned_quiet_pawns.read(m.from());
      }

      if (m.is_capture())
        return valid_attack && !dst.is_empty() && dst.color() != m_stm;
      return rank_delta == 1 && same_file && dst.is_empty() && !pinned_quiet_pawns.read(m.from());
    }

    if (m.flags() == MoveFlags::cap_normal)
      return valid_attack && !dst.is_empty() && dst.color() != m_stm;
    if (m.flags() == MoveFlags::normal)
      return valid_attack && dst.is_empty();

    return false;
  }

  auto Position::has_no_legal_moves_slow() const -> bool {
    const MoveList moves = generate_all_moves(*this);
    return moves.size() == 0;
  }

  auto Position::is_stalemate_slow() const -> bool {
    return !is_in_check() && has_no_legal_moves_slow();
  }

  auto Position::is_fifty_move_draw(i32 ply) const -> std::optional<Score> {
    if (fifty_move_clock() < 100)
      return std::nullopt;
    if (is_in_check() && has_no_legal_moves_slow()) [[unlikely]]
      return score::mated(ply);
    return 0;
  }

  auto Position::is_repetition(const std::vector<u64>& hash_stack, usize hash_waterline) const -> bool {
    const int height = static_cast<int>(hash_stack.size()) - 1;
    usize clones = 0;
    for (int i = height - 4; i >= 0; i -= 2) {
      if (hash_stack[i] == m_hash) {
        const usize clone_limit = i < hash_waterline ? 2 : 1;
        clones++;
        if (clones >= clone_limit)
          return true;
      }
    }
    return false;
  }

  template<eval::concepts::Observer Observer>
  auto Position::move(Move m, Observer observer) const -> Position {
    Position new_pos = *this;
    new_pos.m_valid_pin_info = false;
    new_pos.m_cached_pinned = {};
    new_pos.m_cached_masked_attack_table = {};

    if (new_pos.m_enpassant.is_valid()) {
      new_pos.m_hash ^= hash::enpassant_table[new_pos.m_enpassant.file()];
      new_pos.m_enpassant = Square::invalid();
    }

    const Square from = m.from();
    const Square to = m.to();
    const Place src_place = m_board[from];
    const Place dest_place = m_board[to];
    const PieceId src_id = src_place.id();
    const PieceId dest_id = dest_place.id();

    if (src_place.ptype() == PieceType::k) {
      observer.on_king_move(new_pos, m_stm, from, to);
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
      new_pos.move_piece<false>(observer, m_stm, from, to, src_id, src_place.ptype(), src_place.ptype());
      observer.on_move(*this, m_stm, src_place.ptype(), from, to);
      new_pos.m_hash ^= hash::move_piece(from, to, src_place);
      if (src_place.ptype() != PieceType::p) {
        new_pos.m_50mr++;
      } else {
        new_pos.m_50mr = 0;
      }
      check_src_castling_rights();
    };

    const auto cap_normal = [&] {
      new_pos.remove_piece<false>(observer, !m_stm, to, dest_id, dest_place.ptype());
      observer.on_remove(*this, !m_stm, dest_place.ptype(), to);
      new_pos.move_piece<true>(observer, m_stm, from, to, src_id, src_place.ptype(), src_place.ptype());
      observer.on_move(*this, m_stm, src_place.ptype(), from, to);
      new_pos.m_hash ^= hash::remove_piece(to, dest_place);
      new_pos.m_hash ^= hash::move_piece(from, to, src_place);
      new_pos.m_50mr = 0;
      check_src_castling_rights();
      check_dest_castling_rights();
    };

    const auto promo = [&](auto ptype) {
      new_pos.move_piece<false>(observer, m_stm, from, to, src_id, src_place.ptype(), decltype(ptype)::value);
      observer.on_promote(*this, m_stm, decltype(ptype)::value, from, to);
      new_pos.m_hash ^= hash::promo(from, to, m_stm, decltype(ptype)::value);
      new_pos.m_50mr = 0;
    };

    const auto cap_promo = [&](auto ptype) {
      new_pos.remove_piece<false>(observer, !m_stm, to, dest_id, dest_place.ptype());
      observer.on_remove(*this, !m_stm, dest_place.ptype(), to);
      new_pos.move_piece<true>(observer, m_stm, from, to, src_id, src_place.ptype(), decltype(ptype)::value);
      observer.on_promote(*this, m_stm, decltype(ptype)::value, from, to);
      new_pos.m_hash ^= hash::remove_piece(to, dest_place);
      new_pos.m_hash ^= hash::promo(from, to, m_stm, decltype(ptype)::value);
      new_pos.m_50mr = 0;
      check_dest_castling_rights();
    };

    const auto double_push = [&] {
      new_pos.move_piece<false>(observer, m_stm, from, to, src_id, src_place.ptype(), src_place.ptype());
      observer.on_move(*this, m_stm, src_place.ptype(), from, to);
      new_pos.m_hash ^= hash::move_piece(from, to, src_place);
      new_pos.m_50mr = 0;
      new_pos.m_enpassant = Square {narrow_cast<u8>((from.raw + to.raw) >> 1)};
      new_pos.m_hash ^= hash::enpassant_table[new_pos.m_enpassant.file()];
    };

    const auto enpassant = [&] {
      const Square victim = Square::from_file_and_rank(to.file(), from.rank());
      const PieceId victim_id = m_board[victim].id();

      new_pos.remove_piece<true>(observer, !m_stm, victim, victim_id, PieceType::p);
      observer.on_remove(*this, !m_stm, PieceType::p, victim);
      new_pos.move_piece<false>(observer, m_stm, from, to, src_id, src_place.ptype(), src_place.ptype());
      observer.on_move(*this, m_stm, src_place.ptype(), from, to);
      new_pos.m_hash ^= hash::remove_piece(victim, !m_stm, PieceType::p);
      new_pos.m_hash ^= hash::move_piece(from, to, src_place);

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

      new_pos.remove_piece<true>(observer, m_stm, king_src, king_id, PieceType::k);
      observer.on_remove(*this, m_stm, PieceType::k, king_src);
      new_pos.remove_piece<true>(observer, m_stm, rook_src, rook_id, PieceType::r);
      observer.on_remove(*this, m_stm, PieceType::r, rook_src);
      new_pos.add_piece<true>(observer, m_stm, king_dest, king_id, PieceType::k);
      observer.on_add(*this, m_stm, PieceType::k, king_dest);
      new_pos.add_piece<true>(observer, m_stm, rook_dest, rook_id, PieceType::r);
      observer.on_add(*this, m_stm, PieceType::r, rook_dest);

      new_pos.m_hash ^= hash::move_piece(king_src, king_dest, m_stm, PieceType::k);
      new_pos.m_hash ^= hash::move_piece(rook_src, rook_dest, m_stm, PieceType::r);

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

    new_pos.m_hash ^= hash::castle_table[m_rook_info.to_index()];
    new_pos.m_hash ^= hash::castle_table[new_pos.m_rook_info.to_index()];
    new_pos.m_hash ^= hash::move;

    new_pos.m_ply++;
    new_pos.m_stm = !m_stm;

    observer.on_finalize(new_pos);

    rose_assert(new_pos.m_hash == new_pos.calc_hash_slow(),
                "{} [{:016x}] : {} : {} [{:016x} {:016x}]",
                to_string(MoveFormat::frc),
                m_hash,
                m.to_string(MoveFormat::frc),
                new_pos.to_string(MoveFormat::frc),
                new_pos.m_hash,
                new_pos.calc_hash_slow());

    return new_pos;
  }

  template auto Position::move<eval::NullObserver>(Move m, eval::NullObserver observer) const -> Position;
#define rose_position_move_template(e, T)                                                                                                            \
  template auto Position::move<eval::nnue::T::Observer>(Move m, eval::nnue::T::Observer observer) const -> Position;
  rose_for_each_arch(rose_position_move_template);

  auto Position::null_move() const -> Position {
    Position new_pos = *this;
    new_pos.m_valid_pin_info = false;
    new_pos.m_cached_pinned = {};
    new_pos.m_cached_masked_attack_table = {};

    if (new_pos.m_enpassant.is_valid()) {
      new_pos.m_hash ^= hash::enpassant_table[new_pos.m_enpassant.file()];
      new_pos.m_enpassant = Square::invalid();
    }

    new_pos.m_50mr++;

    new_pos.m_hash ^= hash::move;

    new_pos.m_ply++;
    new_pos.m_stm = !m_stm;

    rose_assert(new_pos.m_hash == new_pos.calc_hash_slow(),
                "{} [{:016x}] : {} : (null move) [{:016x} {:016x}]",
                to_string(MoveFormat::frc),
                m_hash,
                new_pos.to_string(MoveFormat::frc),
                new_pos.m_hash,
                new_pos.calc_hash_slow());

    return new_pos;
  }

  auto Position::calc_hash_slow() const -> Hash {
    Hash result = 0;
    for (u8 sq = 0; sq < 64; sq++)
      result ^= hash::piece_table[m_board[Square {sq}].raw >> 4][sq];
    if (m_enpassant.is_valid())
      result ^= hash::enpassant_table[m_enpassant.file()];
    result ^= hash::castle_table[m_rook_info.to_index()];
    if (m_stm == Color::black)
      result ^= hash::move;
    return result;
  }

  auto Position::pin_info() const -> std::tuple<const std::array<PieceMask, 64>&, Bitboard> {
    if (!m_valid_pin_info) {
      m_valid_pin_info = true;
      std::tie(m_cached_masked_attack_table, m_cached_pinned) = calc_pin_info_slow();
    }
    return std::tie(m_cached_masked_attack_table, m_cached_pinned);
  }

  auto Position::calc_pin_info_slow() const -> std::tuple<std::array<PieceMask, 64>, Bitboard> {
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
    const m8x64 has_attacker = geometry::ray_fill(attackers);
    const m8x64 pinned = potentially_pinned.andnot(enemy) & has_attacker;

    // Translate to valid move rays
    const u8x64 nonmasked_pinned_ids = geometry::lane_broadcast(pinned.mask(ray_places));
    const u8x64 pinned_ids = pin_raymasks.mask(nonmasked_pinned_ids);
    const u8x64 board_layout = (~iperm.msb()).mask(iperm.swizzle(pinned_ids));

    // Convert from list of squares to piecemask
#if LPS_AVX512
    const int pinned_count = pinned.popcount();
    const u8x16 pinned_coord = pinned.compress(ray_coords).extract_aligned<u8x16, 0>();
    const PieceMask piece_mask = geometry::find_set(pinned_coord, pinned_count, piece_list_sq(m_stm).to_vector());
#else
    const PieceMask piece_mask {
      static_cast<u16>((u64x8::splat(1) << (std::bit_cast<u64x8>(nonmasked_pinned_ids) & u64x8::splat(0xF))).reduce_or() & ~1)};
#endif

    // Generate attack table mask
    const u16x64 bits = id_to_at(board_layout);
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
            const Place place = result.m_board[rook_sq];
            if (place.color() == color && place.ptype() == PieceType::r)
              return castle_rights(color, file);
            if (place.color() == color && place.ptype() == PieceType::k)
              return ParseError::invalid_board;
            file += direction;
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
    result.m_hash = result.calc_hash_slow();

    return result;
  }

  auto Position::dump() const -> void {
    fmt::print("fen: {}\n", to_string(MoveFormat::frc));
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
      const auto [at, pinned] = calc_pin_info_slow();
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

  auto Position::to_string(MoveFormat format) const -> std::string {
    std::string result;

    int blanks = 0;
    const auto emit_blanks = [&] {
      if (blanks != 0) {
        fmt::format_to(std::back_inserter(result), "{}", blanks);
        blanks = 0;
      }
    };

    for (i8 rank = 7; rank >= 0; rank--) {
      for (i8 file = 0; file < 8; file++) {
        const Square sq = Square::from_file_and_rank(file, rank);
        const Place p = m_board[sq];
        if (p.is_empty()) {
          blanks++;
        } else {
          emit_blanks();
          fmt::format_to(std::back_inserter(result), "{}", p);
        }
      }
      emit_blanks();
      if (rank != 0) {
        fmt::format_to(std::back_inserter(result), "/");
      }
    }

    fmt::format_to(std::back_inserter(result), " {} ", m_stm);

    if (m_rook_info.is_clear()) {
      fmt::format_to(std::back_inserter(result), "-");
    }
    if (m_rook_info.hside(Color::white).is_valid()) {
      const bool is_classical = format == MoveFormat::classical && m_rook_info.hside(Color::white).file() == 7;
      fmt::format_to(std::back_inserter(result), "{}", is_classical ? 'K' : static_cast<char>('A' + m_rook_info.hside(Color::white).file()));
    }
    if (m_rook_info.aside(Color::white).is_valid()) {
      const bool is_classical = format == MoveFormat::classical && m_rook_info.aside(Color::white).file() == 0;
      fmt::format_to(std::back_inserter(result), "{}", is_classical ? 'Q' : static_cast<char>('A' + m_rook_info.aside(Color::white).file()));
    }
    if (m_rook_info.hside(Color::black).is_valid()) {
      const bool is_classical = format == MoveFormat::classical && m_rook_info.hside(Color::black).file() == 7;
      fmt::format_to(std::back_inserter(result), "{}", is_classical ? 'k' : static_cast<char>('a' + m_rook_info.hside(Color::black).file()));
    }
    if (m_rook_info.aside(Color::black).is_valid()) {
      const bool is_classical = format == MoveFormat::classical && m_rook_info.aside(Color::black).file() == 0;
      fmt::format_to(std::back_inserter(result), "{}", is_classical ? 'q' : static_cast<char>('a' + m_rook_info.aside(Color::black).file()));
    }

    if (m_enpassant.is_valid()) {
      fmt::format_to(std::back_inserter(result), " {} ", m_enpassant);
    } else {
      fmt::format_to(std::back_inserter(result), " - ");
    }

    fmt::format_to(std::back_inserter(result), "{} {}", m_50mr, full_move_counter());

    return result;
  }

}  // namespace rose
