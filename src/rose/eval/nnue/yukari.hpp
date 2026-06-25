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
#include <lps/lps.hpp>

namespace rose::eval::nnue {

  template<usize hl_size>
  struct Yukari {
    inline static constexpr i32 scale = 400;
    inline static constexpr i32 qa = 255;
    inline static constexpr i32 qb = 64;
    inline static constexpr usize output_bucket_count = 8;

    inline static auto psq_feature_index(const Position& pos, Color perspective, Square sq, PieceType ptype, Color side) -> usize {
      //                          qrb-npk-
      constexpr u32 ptype_lut = 0x43201050;

      usize side_index = side.to_index();
      usize ptype_index = (ptype_lut >> (4 * ptype.to_index())) & 0xf;
      usize square_index = sq.raw ^ (pos.king_sq(perspective).file() >= 4 ? 0b000111 : 0);

      if (perspective == Color::black) {
        side_index ^= 1;
        square_index ^= 0b111000;
      }

      return side_index * 64 * 6 + ptype_index * 64 + square_index;
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

    inline static auto threat_add(const Network& net, Accumulator& acc0, usize feat0, Accumulator& acc1, usize feat1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i8xhN::load(&net.pst_weights[feat0][i]).template convert<i16>();
        const i16xN w1 = i8xhN::load(&net.pst_weights[feat1][i]).template convert<i16>();
        (i16xN::load(&acc0[i]) + w0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) + w1).store(&acc1[i]);
      }
    }

    inline static auto threat_sub(const Network& net, Accumulator& acc0, usize feat0, Accumulator& acc1, usize feat1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i8xhN::load(&net.pst_weights[feat0][i]).template convert<i16>();
        const i16xN w1 = i8xhN::load(&net.pst_weights[feat1][i]).template convert<i16>();
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
        if (refresh) {
          m_accum = rebuild_accumulator(pos, m_net);
        }
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
