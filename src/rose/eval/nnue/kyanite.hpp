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

    struct AccumulatorPair {
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

    inline static auto add(const Network& net, Accumulator& accumulator, usize feature) -> void {
      for (usize i = 0; i < hl_size; i++)
        accumulator[i] += net.accumulator_weights[feature][i];
    }

    inline static auto sub(const Network& net, Accumulator& accumulator, usize feature) -> void {
      for (usize i = 0; i < hl_size; i++)
        accumulator[i] -= net.accumulator_weights[feature][i];
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

        add(net, result.values[0], feature0);
        add(net, result.values[1], feature1);
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
        add(m_net, m_accum.values[0], feature_index(pos, Color::white, sq, ptype, side));
        add(m_net, m_accum.values[1], feature_index(pos, Color::black, sq, ptype, side));
      }

      auto on_remove(const Position& pos, Color side, PieceType ptype, Square sq) -> void {
        sub(m_net, m_accum.values[0], feature_index(pos, Color::white, sq, ptype, side));
        sub(m_net, m_accum.values[1], feature_index(pos, Color::black, sq, ptype, side));
      }

      auto on_mutate(const Position& pos, Color side, PieceType src_ptype, PieceType dst_ptype, Square sq) -> void {
        on_remove(pos, side, src_ptype, sq);
        on_add(pos, side, dst_ptype, sq);
      }

      auto on_move(const Position& pos, Color side, PieceType ptype, Square from, Square to) -> void {
        on_remove(pos, side, ptype, from);
        on_add(pos, side, ptype, to);
      }

      auto on_promote(const Position& pos, Color side, PieceType dst_ptype, Square from, Square to) -> void {
        on_remove(pos, side, PieceType::p, from);
        on_add(pos, side, dst_ptype, to);
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

      auto evaluate(const Accumulator& us, const Accumulator& them) -> i32 {
        i32 output = 0;
#if LPS_AVX512
        const __m512i vec_zero = _mm512_setzero_si512();
        const __m512i vec_qa = _mm512_set1_epi16(qa);
        __m512i vec_output0 = _mm512_setzero_si512();
        __m512i vec_output1 = _mm512_setzero_si512();
        for (usize i = 0; i < hl_size; i += 32) {
          // Weights
          const __m512i w0 = _mm512_loadu_si512((__m512i const*)&m_net.output_weights[0][i]);
          const __m512i w1 = _mm512_loadu_si512((__m512i const*)&m_net.output_weights[1][i]);
          // Inputs
          const __m512i x0_i = _mm512_loadu_si512((__m512i const*)&us[i]);
          const __m512i x1_i = _mm512_loadu_si512((__m512i const*)&them[i]);
          // Clipped inputs
          const __m512i c0_i = _mm512_min_epi16(_mm512_max_epi16(x0_i, vec_zero), vec_qa);
          const __m512i c1_i = _mm512_min_epi16(_mm512_max_epi16(x1_i, vec_zero), vec_qa);
          // w * c_i * c_i + accumulate
          vec_output0 = _mm512_dpwssd_epi32(vec_output0, _mm512_mullo_epi16(w0, c0_i), c0_i);
          vec_output1 = _mm512_dpwssd_epi32(vec_output1, _mm512_mullo_epi16(w1, c1_i), c1_i);
        }
        // Horizontal sum
        __m512i vec_output = _mm512_add_epi32(vec_output0, vec_output1);
        __m256i hsum256 = _mm256_add_epi32(_mm512_extracti32x8_epi32(vec_output, 1), _mm512_castsi512_si256(vec_output));
        __m128i hsum = _mm_add_epi32(_mm256_extracti128_si256(hsum256, 1), _mm256_castsi256_si128(hsum256));
        hsum = _mm_add_epi32(hsum, _mm_unpackhi_epi64(hsum, hsum));
        hsum = _mm_add_epi32(hsum, _mm_shuffle_epi32(hsum, 1));
        output = _mm_cvtsi128_si32(hsum);
#elif LPS_AVX2
        const __m256i vec_zero = _mm256_setzero_si256();
        const __m256i vec_qa = _mm256_set1_epi16(qa);
        __m256i vec_output0 = _mm256_setzero_si256();
        __m256i vec_output1 = _mm256_setzero_si256();
        for (usize i = 0; i < hl_size; i += 16) {
          // Weights
          const __m256i w0 = _mm256_loadu_si256((__m256i const*)&m_net.output_weights[0][i]);
          const __m256i w1 = _mm256_loadu_si256((__m256i const*)&m_net.output_weights[1][i]);
          // Inputs
          const __m256i x0_i = _mm256_loadu_si256((__m256i const*)&us[i]);
          const __m256i x1_i = _mm256_loadu_si256((__m256i const*)&them[i]);
          // Clipped inputs
          const __m256i c0_i = _mm256_min_epi16(_mm256_max_epi16(x0_i, vec_zero), vec_qa);
          const __m256i c1_i = _mm256_min_epi16(_mm256_max_epi16(x1_i, vec_zero), vec_qa);
          // w * c_i * c_i
          const __m256i y0_i = _mm256_madd_epi16(_mm256_mullo_epi16(w0, c0_i), c0_i);
          const __m256i y1_i = _mm256_madd_epi16(_mm256_mullo_epi16(w1, c1_i), c1_i);
          // Accumulate
          vec_output0 = _mm256_add_epi32(vec_output0, y0_i);
          vec_output1 = _mm256_add_epi32(vec_output1, y1_i);
        }
        // Horizontal sum
        __m256i vec_output = _mm256_add_epi32(vec_output0, vec_output1);
        __m128i hsum = _mm_add_epi32(_mm256_extracti128_si256(vec_output, 1), _mm256_castsi256_si128(vec_output));
        hsum = _mm_add_epi32(hsum, _mm_unpackhi_epi64(hsum, hsum));
        hsum = _mm_add_epi32(hsum, _mm_shuffle_epi32(hsum, 1));
        output = _mm_cvtsi128_si32(hsum);
#else
        for (usize i = 0; i < hl_size; i++) {
          output += screlu(us[i]) * m_net.output_weights[0][i];
          output += screlu(them[i]) * m_net.output_weights[1][i];
        }
#endif
        output /= qa;
        output += m_net.output_bias;
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

        rose_assert(rebuild_accumulator(pos, m_net) == accumulators);

        return evaluate(accumulators.get(stm), accumulators.get(stm.invert()));
      }

      auto observer() -> Observer {
        return Observer {m_net, m_stack.back()};
      }
    };
  };

}  // namespace rose::eval::nnue
