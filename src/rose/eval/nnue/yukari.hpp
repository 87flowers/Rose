#pragma once

#include "rose/common.hpp"
#include "rose/eval/concepts.hpp"
#include "rose/limits.hpp"
#include "rose/position.hpp"
#include "rose/score.hpp"
#include "rose/square.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/static_vector.hpp"

#include <array>
#include <bit>
#include <lps/lps.hpp>

namespace rose::eval::nnue {

  template<usize hl_size>
  struct Yukari {
    inline static constexpr i32 scale = 400;
    inline static constexpr i32 qa = 255;
    inline static constexpr i32 qb = 64;
    inline static constexpr usize output_bucket_count = 8;

    inline static auto king_mirror(const Position& pos, Color perspective) -> usize {
      return pos.king_sq(perspective).file() >= 4 ? 0b000111 : 0;
    }

    inline static auto psq_feature_index(const Position& pos, Color perspective, Square sq, PieceType ptype, Color side) -> usize {
      //                          qrb-npk-
      constexpr u32 ptype_lut = 0x43201050;

      usize side_index = side.to_index();
      usize ptype_index = (ptype_lut >> (4 * ptype.to_index())) & 0xf;
      usize square_index = sq.to_index() ^ king_mirror(pos, perspective);

      if (perspective == Color::black) {
        side_index ^= 1;
        square_index ^= 0b111000;
      }

      return side_index * 64 * 6 + ptype_index * 64 + square_index;
    }

    template<PieceType ptype>
    inline static constexpr auto attacks_array_for_piece() -> std::array<Bitboard, 64> {
      std::array<Bitboard, 64> attacks {};
      for (u8 i = 0; i < 64; ++i) {
        const Square sq {i};
        const Bitboard sq_bb = sq.to_bitboard();

        auto gen = [&](std::initializer_list<Direction> dirs) -> Bitboard {
          Bitboard attacks {};
          for (const Direction dir : dirs) {
            Bitboard bb = sq_bb.shift(dir);
            while (!bb.is_empty()) {
              attacks |= bb;
              bb = bb.shift(dir);
            }
          }
          return attacks;
        };

        switch (ptype.raw) {
        case PieceType::n:
          attacks[i] =
            sq_bb.shift(Direction::n).shift(Direction::n).shift(Direction::e) | sq_bb.shift(Direction::n).shift(Direction::n).shift(Direction::w) |
            sq_bb.shift(Direction::e).shift(Direction::e).shift(Direction::n) | sq_bb.shift(Direction::e).shift(Direction::e).shift(Direction::s) |
            sq_bb.shift(Direction::s).shift(Direction::s).shift(Direction::e) | sq_bb.shift(Direction::s).shift(Direction::s).shift(Direction::w) |
            sq_bb.shift(Direction::w).shift(Direction::w).shift(Direction::n) | sq_bb.shift(Direction::w).shift(Direction::w).shift(Direction::s);
          break;
        case PieceType::b:
          attacks[i] = gen({Direction::ne, Direction::nw, Direction::se, Direction::sw});
          break;
        case PieceType::r:
          attacks[i] = gen({Direction::n, Direction::e, Direction::s, Direction::w});
          break;
        case PieceType::q:
          attacks[i] = gen({Direction::n, Direction::e, Direction::s, Direction::w, Direction::ne, Direction::nw, Direction::se, Direction::sw});
          break;
        default:
          rose_assert(false);
        }
      }
      return attacks;
    }

    template<PieceType ptype>
    inline static constexpr auto index_array_for_piece() -> std::array<usize, 65> {
      constexpr std::array<Bitboard, 64> attacks = attacks_array_for_piece<ptype>();
      std::array<usize, 65> indexes {};
      for (i32 i = 0; i < 64; i++) {
        indexes[i + 1] = indexes[i] + attacks[i].popcount();
      }
      return indexes;
    }

    inline static constexpr usize pawn_index = 84;
    inline static constexpr std::array<usize, 65> knight_index = index_array_for_piece<PieceType::n>();
    inline static constexpr std::array<usize, 65> bishop_index = index_array_for_piece<PieceType::b>();
    inline static constexpr std::array<usize, 65> rook_index = index_array_for_piece<PieceType::r>();
    inline static constexpr std::array<usize, 65> queen_index = index_array_for_piece<PieceType::q>();

    inline static constexpr usize pawn_offset = 0;
    inline static constexpr usize knight_offset = pawn_offset + 6 * pawn_index;
    inline static constexpr usize bishop_offset = knight_offset + 10 * knight_index[64];
    inline static constexpr usize rook_offset = bishop_offset + 8 * bishop_index[64];
    inline static constexpr usize queen_offset = rook_offset + 8 * rook_index[64];
    inline static constexpr usize max_offset = queen_offset + 10 * queen_index[64];

    inline static constexpr std::array<std::array<i32, 8>, 2> pawn_map {{
      {{-1, -1, 0, 1, -1, -1, 2, -1}},
      {{-1, -1, 3, 4, -1, -1, 5, -1}},
    }};
    inline static constexpr std::array<std::array<i32, 8>, 2> knight_queen_map {{
      {{-1, -1, 0, 1, -1, 2, 3, 4}},
      {{-1, -1, 5, 6, -1, 7, 8, 9}},
    }};
    inline static constexpr std::array<std::array<i32, 8>, 2> bishop_rook_map {{
      {{-1, -1, 0, 1, -1, 2, 3, -1}},
      {{-1, -1, 4, 5, -1, 6, 7, -1}},
    }};

    inline static constexpr std::array<Bitboard, 64> knight_attacks = attacks_array_for_piece<PieceType::n>();
    inline static constexpr std::array<Bitboard, 64> bishop_attacks = attacks_array_for_piece<PieceType::b>();
    inline static constexpr std::array<Bitboard, 64> rook_attacks = attacks_array_for_piece<PieceType::r>();
    inline static constexpr std::array<Bitboard, 64> queen_attacks = attacks_array_for_piece<PieceType::q>();

    inline static auto threat_feature_index(const Position& pos, Color perspective, Square from, Square to) -> std::optional<usize> {
      const Place src = pos.place_at(from);
      const Place dst = pos.place_at(to);

      const bool friendly = src.color() == perspective;
      const bool attacking_enemy = dst.color() != perspective;

      if (src.ptype() == PieceType::k || dst.ptype() == PieceType::k)
        return std::nullopt;

      i64 from_index = from.to_index();
      i64 to_index = to.to_index();
      if (perspective == Color::black) {
        from_index ^= 0b111000;
        to_index ^= 0b111000;
      }
      from_index ^= king_mirror(pos, perspective);
      to_index ^= king_mirror(pos, perspective);

      if (friendly == attacking_enemy && to_index > from_index && src.ptype() == dst.ptype()) {
        return std::nullopt;
      }

      const auto pawn_threat = [&]() -> std::optional<usize> {
        const i32 map = pawn_map[attacking_enemy][dst.ptype().to_index()];
        if (map < 0)
          return std::nullopt;

        const bool up = to_index > from_index;
        const u32 diff = std::abs(to_index - from_index);
        const bool id = diff != std::array<u32, 2> {9, 7}[up];
        const usize attack = 2 * (from_index % 8) + id - 1;
        return pawn_offset + map * pawn_index + (from_index / 8 - 1) * 14 + attack + !friendly * max_offset;
      };

      const auto piece_threat = [&](const std::array<usize, 65>& indexes,
                                    const std::array<Bitboard, 64>& attacks,
                                    const std::array<std::array<i32, 8>, 2>& piece_map,
                                    usize offset) -> std::optional<usize> {
        const i32 map = piece_map[attacking_enemy][dst.ptype().to_index()];
        if (map < 0)
          return std::nullopt;

        const u32 below = std::popcount(attacks[from_index].raw & ((u64 {1} << to_index) - 1));
        return offset + map * indexes[64] + indexes[from_index] + below + !friendly * max_offset;
      };

      switch (src.ptype().raw) {
      case PieceType::p:
        return pawn_threat();
      case PieceType::n:
        return piece_threat(knight_index, knight_attacks, knight_queen_map, knight_offset);
      case PieceType::b:
        return piece_threat(bishop_index, bishop_attacks, bishop_rook_map, bishop_offset);
      case PieceType::r:
        return piece_threat(rook_index, rook_attacks, bishop_rook_map, rook_offset);
      case PieceType::q:
        return piece_threat(queen_index, queen_attacks, knight_queen_map, queen_offset);
      case PieceType::k:
        return std::nullopt;
      case PieceType::none:
        return std::nullopt;
      }
    }

    using Accumulator = std::array<i16, hl_size>;

    struct alignas(64) AccumulatorPair {
      std::array<Accumulator, 2> values;

      const Accumulator& get(Color color) const {
        return values[color.to_index()];
      }

      inline constexpr auto operator==(const AccumulatorPair&) const -> bool = default;
    };

    struct Network {
      std::array<std::array<i8, hl_size>, 60144> threat_weights;
      std::array<Accumulator, 768> pst_weights;
      Accumulator biases;
      std::array<std::array<Accumulator, 2>, output_bucket_count> output_weights;
      std::array<i16, output_bucket_count> output_biases;
    };

    inline static auto pst_add(const Network& net, Accumulator& acc0, usize feat0, Accumulator& acc1, usize feat1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i16xN::load(&net.pst_weights[feat0][i]);
        const i16xN w1 = i16xN::load(&net.pst_weights[feat1][i]);
        (i16xN::load(&acc0[i]) + w0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) + w1).store(&acc1[i]);
      }
    }

    inline static auto pst_sub(const Network& net, Accumulator& acc0, usize feat0, Accumulator& acc1, usize feat1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i16xN::load(&net.pst_weights[feat0][i]);
        const i16xN w1 = i16xN::load(&net.pst_weights[feat1][i]);
        (i16xN::load(&acc0[i]) - w0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) - w1).store(&acc1[i]);
      }
    }

    inline static auto pst_subadd(const Network& net, Accumulator& acc0, usize sub0, usize add0, Accumulator& acc1, usize sub1, usize add1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w_sub0 = i16xN::load(&net.pst_weights[sub0][i]);
        const i16xN w_sub1 = i16xN::load(&net.pst_weights[sub1][i]);
        const i16xN w_add0 = i16xN::load(&net.pst_weights[add0][i]);
        const i16xN w_add1 = i16xN::load(&net.pst_weights[add1][i]);
        (i16xN::load(&acc0[i]) - w_sub0 + w_add0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) - w_sub1 + w_add1).store(&acc1[i]);
      }
    }

    using i8xhN = lps::environment::vector<i8, std::max<usize>(16, i16xN::size)>;

    inline static auto threat_add(const Network& net, Accumulator& acc0, usize feat0) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i8xhN::load(&net.threat_weights[feat0][i]).template convert<i16>();
        (i16xN::load(&acc0[i]) + w0).store(&acc0[i]);
      }
    }

    inline static auto threat_add(const Network& net, Accumulator& acc0, usize feat0, Accumulator& acc1, usize feat1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i8xhN::load(&net.threat_weights[feat0][i]).template convert<i16>();
        const i16xN w1 = i8xhN::load(&net.threat_weights[feat1][i]).template convert<i16>();
        (i16xN::load(&acc0[i]) + w0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) + w1).store(&acc1[i]);
      }
    }

    inline static auto threat_sub(const Network& net, Accumulator& acc0, usize feat0, Accumulator& acc1, usize feat1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i8xhN::load(&net.threat_weights[feat0][i]).template convert<i16>();
        const i16xN w1 = i8xhN::load(&net.threat_weights[feat1][i]).template convert<i16>();
        (i16xN::load(&acc0[i]) - w0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) - w1).store(&acc1[i]);
      }
    }

    inline static auto rebuild_accumulator(const Position& pos, const Network& net) -> AccumulatorPair {
      AccumulatorPair result;
      result.values.fill(net.biases);

      for (u8 i = 0; i < 64; i++) {
        const Square sq {i};
        const Place p = pos.place_at(sq);

        if (p.is_empty())
          continue;

        const usize feature0 = psq_feature_index(pos, Color::white, sq, p.ptype(), p.color());
        const usize feature1 = psq_feature_index(pos, Color::black, sq, p.ptype(), p.color());

        pst_add(net, result.values[0], feature0, result.values[1], feature1);
      }

      for (Color c : {Color::white, Color::black}) {
        for (Square to : pos.attack_table(c).bitboard_any() & pos.board().occupied_bitboard()) {
          for (PieceId src : pos.attack_table(c).read(to)) {
            const Square from = pos.where_is(c, src);
            if (const auto feature0 = threat_feature_index(pos, Color::white, from, to)) {
              threat_add(net, result.values[0], *feature0);
              // fmt::print("white: {} {} {} {} {}\n", pos.place_at(from), from, pos.place_at(to), to, *feature0);
            }
            if (const auto feature1 = threat_feature_index(pos, Color::black, from, to)) {
              threat_add(net, result.values[1], *feature1);
              // fmt::print("black: {} {} {} {} {}\n", pos.place_at(from), from, pos.place_at(to), to, *feature1);
            }
          }
        }
      }

      return result;
    }

    struct Observer {
    private:
      const Network& m_net;
      AccumulatorPair& m_accum;
      bool refresh = false;

    public:
      Observer(const Network& net, AccumulatorPair& accum) :
          m_net(net),
          m_accum(accum) {
      }

      auto on_king_move(const Position& pos, Color stm, Square from, Square to) -> void {
        refresh = (from.file() >= 4 && to.file() < 4) || (from.file() < 4 && to.file() >= 4);
      }

      auto on_add(const Position& pos, Color side, PieceType ptype, Square sq) -> void {
        pst_add(m_net,
                m_accum.values[0],
                psq_feature_index(pos, Color::white, sq, ptype, side),
                m_accum.values[1],
                psq_feature_index(pos, Color::black, sq, ptype, side));
      }

      auto on_remove(const Position& pos, Color side, PieceType ptype, Square sq) -> void {
        pst_sub(m_net,
                m_accum.values[0],
                psq_feature_index(pos, Color::white, sq, ptype, side),
                m_accum.values[1],
                psq_feature_index(pos, Color::black, sq, ptype, side));
      }

      auto on_mutate(const Position& pos, Color side, PieceType src_ptype, PieceType dst_ptype, Square sq) -> void {
        pst_subadd(m_net,
                   m_accum.values[0],
                   psq_feature_index(pos, Color::white, sq, src_ptype, side),
                   psq_feature_index(pos, Color::white, sq, dst_ptype, side),
                   m_accum.values[1],
                   psq_feature_index(pos, Color::black, sq, src_ptype, side),
                   psq_feature_index(pos, Color::black, sq, dst_ptype, side));
      }

      auto on_move(const Position& pos, Color side, PieceType ptype, Square from, Square to) -> void {
        pst_subadd(m_net,
                   m_accum.values[0],
                   psq_feature_index(pos, Color::white, from, ptype, side),
                   psq_feature_index(pos, Color::white, to, ptype, side),
                   m_accum.values[1],
                   psq_feature_index(pos, Color::black, from, ptype, side),
                   psq_feature_index(pos, Color::black, to, ptype, side));
      }

      auto on_promote(const Position& pos, Color side, PieceType dst_ptype, Square from, Square to) -> void {
        pst_subadd(m_net,
                   m_accum.values[0],
                   psq_feature_index(pos, Color::white, from, PieceType::p, side),
                   psq_feature_index(pos, Color::white, to, dst_ptype, side),
                   m_accum.values[1],
                   psq_feature_index(pos, Color::black, from, PieceType::p, side),
                   psq_feature_index(pos, Color::black, to, dst_ptype, side));
      }

      auto on_add_focus_threats(const Position& pos, PieceType ptype, Square sq, u64 attacker_mask, u8x64 ray_coords, u8x64 ray_places) -> void {
        rose_unused(pos, ptype, sq, attacker_mask, ray_coords, ray_places);
      }

      auto on_remove_focus_threats(const Position& pos, PieceType ptype, Square sq) -> void {
        rose_unused(pos, ptype, sq);
      }

      auto on_add_discovered_threats(const Position& pos, m8x64 sliders, m8x64 raymask, u8x64 ray_coords, u8x64 ray_places) -> void {
        rose_unused(pos, sliders, raymask, ray_coords, ray_places);
      }

      auto on_remove_discovered_threats(const Position& pos, m8x64 sliders, m8x64 raymask, u8x64 ray_coords, u8x64 ray_places) -> void {
        rose_unused(pos, sliders, raymask, ray_coords, ray_places);
      }

      auto on_finalize(const Position& pos) -> void {
        m_accum = rebuild_accumulator(pos, m_net);
      }
    };

    static_assert(concepts::Observer<Observer>);

    struct State {
    private:
      StaticVector<AccumulatorPair, max_depth + 6> m_stack;
      const Network& m_net;

      auto evaluate(const Accumulator& us, const Accumulator& them, usize output_bucket) -> i32 {
        static_assert(hl_size % i16xN::size == 0);

        i32xN output0 = i32xN::zero();
        i32xN output1 = i32xN::zero();
        for (usize i = 0; i < hl_size; i += i16xN::size) {
          const i16xN w0 = i16xN::load(&m_net.output_weights[output_bucket][0][i]);
          const i16xN w1 = i16xN::load(&m_net.output_weights[output_bucket][1][i]);
          const i16xN x0_i = i16xN::load(&us[i]);
          const i16xN x1_i = i16xN::load(&them[i]);
          const i16xN c0_i = x0_i.clamp(i16xN::zero(), i16xN::splat(qa));
          const i16xN c1_i = x1_i.clamp(i16xN::zero(), i16xN::splat(qa));
          output0 = output0.accumulate_pair_dot(w0 * c0_i, c0_i);
          output1 = output1.accumulate_pair_dot(w1 * c1_i, c1_i);
        }

        i32 output = (output0 + output1).reduce_add();
        output /= qa;
        output += m_net.output_biases[output_bucket];
        output *= scale;
        output /= qa * qb;
        return output;
      }

    public:
      explicit State(const Network& net) :
          m_net(net) {
      }

      auto reset(const Position& pos) -> void {
        m_stack.clear();
        m_stack.push_back(rebuild_accumulator(pos, m_net));
      }

      auto push() -> void {
        m_stack.push_back(m_stack.back());
      }

      auto pop() -> void {
        m_stack.pop_back();
      }

      auto evaluate(const Position& pos) -> Score {
        const Color stm = pos.stm();
        const AccumulatorPair& accumulators = m_stack.back();
        const usize output_bucket = (pos.piece_count() - 2) / (32 / output_bucket_count);

        rose_assert(rebuild_accumulator(pos, m_net) == accumulators);

        return evaluate(accumulators.get(stm), accumulators.get(stm.invert()), output_bucket);
      }

      auto observer() -> Observer {
        return Observer {m_net, m_stack.back()};
      }
    };
  };

}  // namespace rose::eval::nnue
