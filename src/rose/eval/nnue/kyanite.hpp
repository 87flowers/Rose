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
  struct Kyanite {
    inline static constexpr usize input_size = 768;
    inline static constexpr i32 scale = 400;
    inline static constexpr i32 qa = 255;
    inline static constexpr i32 qb = 64;

    inline static auto feature_index(const Position& pos, Color perspective, Square sq, PieceType ptype, Color side) -> usize {
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
      std::array<Accumulator, input_size> accumulator_weights;
      Accumulator accumulator_biases;
      std::array<Accumulator, 2> output_weights;
      i16 output_bias;
    };

    inline static auto add(const Network& net, Accumulator& acc0, usize feat0, Accumulator& acc1, usize feat1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i16xN::load(&net.accumulator_weights[feat0][i]);
        const i16xN w1 = i16xN::load(&net.accumulator_weights[feat1][i]);
        (i16xN::load(&acc0[i]) + w0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) + w1).store(&acc1[i]);
      }
    }

    inline static auto sub(const Network& net, Accumulator& acc0, usize feat0, Accumulator& acc1, usize feat1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w0 = i16xN::load(&net.accumulator_weights[feat0][i]);
        const i16xN w1 = i16xN::load(&net.accumulator_weights[feat1][i]);
        (i16xN::load(&acc0[i]) - w0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) - w1).store(&acc1[i]);
      }
    }

    inline static auto subadd(const Network& net, Accumulator& acc0, usize sub0, usize add0, Accumulator& acc1, usize sub1, usize add1) -> void {
      static_assert(hl_size % i16xN::size == 0);

      for (usize i = 0; i < hl_size; i += i16xN::size) {
        const i16xN w_sub0 = i16xN::load(&net.accumulator_weights[sub0][i]);
        const i16xN w_sub1 = i16xN::load(&net.accumulator_weights[sub1][i]);
        const i16xN w_add0 = i16xN::load(&net.accumulator_weights[add0][i]);
        const i16xN w_add1 = i16xN::load(&net.accumulator_weights[add1][i]);
        (i16xN::load(&acc0[i]) - w_sub0 + w_add0).store(&acc0[i]);
        (i16xN::load(&acc1[i]) - w_sub1 + w_add1).store(&acc1[i]);
      }
    }

    inline static auto screlu(i16 x) -> i32 {
      i32 y = std::clamp<i32>(x, 0, qa);
      return y * y;
    }

    inline static auto rebuild_accumulator(const Position& pos, const Network& net) -> AccumulatorPair {
      AccumulatorPair result;
      result.values.fill(net.accumulator_biases);

      for (u8 i = 0; i < 64; i++) {
        const Square sq {i};
        const Place p = pos.place_at(sq);

        if (p.is_empty())
          continue;

        const usize feature0 = feature_index(pos, Color::white, sq, p.ptype(), p.color());
        const usize feature1 = feature_index(pos, Color::black, sq, p.ptype(), p.color());

        add(net, result.values[0], feature0, result.values[1], feature1);
      }

      return result;
    }

    struct StackEntry {
      AccumulatorPair accumulators;
      std::array<StaticVector<usize, 2>, Color::count> sub;
      std::array<StaticVector<usize, 2>, Color::count> add;
      bool materialized = false;
      bool needs_rebuild = false;
    };

    struct Observer {
    private:
      const Network& m_net;
      StackEntry& m_entry;
      bool refresh = false;

    public:
      explicit Observer(const Network& net, StackEntry& entry) :
          m_net(net),
          m_entry(entry) {
      }

      auto on_king_move(const Position& pos, Color stm, Square from, Square to) -> void {
        rose_unused(pos, stm);
        m_entry.needs_rebuild = (from.file() >= 4 && to.file() < 4) || (from.file() < 4 && to.file() >= 4);
      }

      auto on_add(const Position& pos, Color side, PieceType ptype, Square sq) -> void {
        m_entry.add[Color::white].push_back(feature_index(pos, Color::white, sq, ptype, side));
        m_entry.add[Color::black].push_back(feature_index(pos, Color::black, sq, ptype, side));
      }

      auto on_remove(const Position& pos, Color side, PieceType ptype, Square sq) -> void {
        m_entry.sub[Color::white].push_back(feature_index(pos, Color::white, sq, ptype, side));
        m_entry.sub[Color::black].push_back(feature_index(pos, Color::black, sq, ptype, side));
      }

      auto on_mutate(const Position& pos, Color side, PieceType src_ptype, PieceType dst_ptype, Square sq) -> void {
        m_entry.sub[Color::white].push_back(feature_index(pos, Color::white, sq, src_ptype, side));
        m_entry.sub[Color::black].push_back(feature_index(pos, Color::black, sq, src_ptype, side));
        m_entry.add[Color::white].push_back(feature_index(pos, Color::white, sq, dst_ptype, side));
        m_entry.add[Color::black].push_back(feature_index(pos, Color::black, sq, dst_ptype, side));
      }

      auto on_move(const Position& pos, Color side, PieceType ptype, Square from, Square to) -> void {
        m_entry.sub[Color::white].push_back(feature_index(pos, Color::white, from, ptype, side));
        m_entry.sub[Color::black].push_back(feature_index(pos, Color::black, from, ptype, side));
        m_entry.add[Color::white].push_back(feature_index(pos, Color::white, to, ptype, side));
        m_entry.add[Color::black].push_back(feature_index(pos, Color::black, to, ptype, side));
      }

      auto on_promote(const Position& pos, Color side, PieceType dst_ptype, Square from, Square to) -> void {
        m_entry.sub[Color::white].push_back(feature_index(pos, Color::white, from, PieceType::p, side));
        m_entry.sub[Color::black].push_back(feature_index(pos, Color::black, from, PieceType::p, side));
        m_entry.add[Color::white].push_back(feature_index(pos, Color::white, to, dst_ptype, side));
        m_entry.add[Color::black].push_back(feature_index(pos, Color::black, to, dst_ptype, side));
      }

      auto on_finalize(const Position& pos) -> void {
        rose_unused(pos);
      }
    };

    static_assert(concepts::Observer<Observer>);

    struct State {
    private:
      StaticVector<StackEntry, max_depth + 6> m_stack;
      const Network& m_net;

      auto evaluate(const Accumulator& us, const Accumulator& them) -> i32 {
        static_assert(hl_size % i16xN::size == 0);

        i32xN output0 = i32xN::zero();
        i32xN output1 = i32xN::zero();
        for (usize i = 0; i < hl_size; i += i16xN::size) {
          const i16xN w0 = i16xN::load(&m_net.output_weights[0][i]);
          const i16xN w1 = i16xN::load(&m_net.output_weights[1][i]);
          const i16xN x0_i = i16xN::load(&us[i]);
          const i16xN x1_i = i16xN::load(&them[i]);
          const i16xN c0_i = x0_i.clamp(i16xN::zero(), i16xN::splat(qa));
          const i16xN c1_i = x1_i.clamp(i16xN::zero(), i16xN::splat(qa));
          output0 = output0.accumulate_pair_dot(w0 * c0_i, c0_i);
          output1 = output1.accumulate_pair_dot(w1 * c1_i, c1_i);
        }

        i32 output = (output0 + output1).reduce_add();
        output /= qa;
        output += m_net.output_bias;
        output *= scale;
        output /= qa * qb;
        return output;
      }

      auto materialize_stack(const Position& current_position) -> void {
        const usize current = m_stack.size() - 1;
        usize i = current;

        while (!m_stack[i].materialized) {
          if (m_stack[i].needs_rebuild) {
            m_stack[current].materialized = true;
            m_stack[current].accumulators = rebuild_accumulator(current_position, m_net);
            return;
          }
          i--;
        }

        rose_assert(m_stack[i].materialized);

        while (i < current) {
          i++;
          materialize_entry(i);
        }
      }

      auto materialize_entry(usize i) -> void {
        rose_assert(m_stack[i - 1].materialized);
        m_stack[i].accumulators = m_stack[i - 1].accumulators;

        const usize add_count = m_stack[i].add[0].size();
        const usize sub_count = m_stack[i].sub[0].size();

        rose_assert(add_count == m_stack[i].add[1].size());
        rose_assert(sub_count == m_stack[i].sub[1].size());

        for (usize j = 0; j < sub_count; j++) {
          sub(m_net, m_stack[i].accumulators.values[0], m_stack[i].sub[0][j], m_stack[i].accumulators.values[1], m_stack[i].sub[1][j]);
        }
        for (usize j = 0; j < add_count; j++) {
          add(m_net, m_stack[i].accumulators.values[0], m_stack[i].add[0][j], m_stack[i].accumulators.values[1], m_stack[i].add[1][j]);
        }

        m_stack[i].materialized = true;
      }

    public:
      explicit State(const Network& net) :
          m_net(net) {
      }

      auto reset(const Position& pos) -> void {
        m_stack.clear();
        m_stack.push_back(StackEntry {
          .accumulators = rebuild_accumulator(pos, m_net),
          .materialized = true,
        });
      }

      auto push() -> void {
        m_stack.push_back({
          .accumulators = m_stack.back().accumulators,
        });
      }

      auto pop() -> void {
        m_stack.pop_back();
      }

      auto evaluate(const Position& pos) -> Score {
        materialize_stack(pos);

        const Color stm = pos.stm();
        const AccumulatorPair& accumulators = m_stack.back().accumulators;

        rose_assert(m_stack.back().materialized);
        rose_assert(rebuild_accumulator(pos, m_net) == accumulators);

        return evaluate(accumulators.get(stm), accumulators.get(stm.invert()));
      }

      auto observer() -> Observer {
        return Observer {m_net, m_stack.back()};
      }
    };
  };

}  // namespace rose::eval::nnue
