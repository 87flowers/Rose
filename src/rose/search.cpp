#include "rose/search.hpp"

#include "rose/common.hpp"
#include "rose/engine_output.hpp"
#include "rose/eval/nnue/arch.hpp"
#include "rose/game.hpp"
#include "rose/limits.hpp"
#include "rose/move_picker.hpp"
#include "rose/movegen.hpp"
#include "rose/node_type.hpp"
#include "rose/score.hpp"
#include "rose/search_control.hpp"
#include "rose/see.hpp"
#include "rose/tt.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/defer.hpp"
#include "rose/util/time.hpp"

#include <atomic>
#include <bit>
#include <fmt/format.h>
#include <memory>
#include <thread>
#include <variant>

namespace rose {

  auto SearchShared::reset() -> void {
    stats.clear();
    transposition_table.clear();
  }

  auto SearchShared::set_hash_size(int mb) -> void {
    rose_assert(mb > 0);
    transposition_table.resize(static_cast<usize>(mb));
  }

  auto SearchShared::set_output(std::shared_ptr<EngineOutput> output) -> void {
    this->output = output;
  }

  auto SearchShared::send_ping() -> void {
    engine_message = EngineMessage::ping;
    idle_barrier.arrive_and_wait();
    started_barrier.arrive_and_wait();
  }

  auto SearchShared::send_quit() -> void {
    engine_message = EngineMessage::quit;
    idle_barrier.arrive_and_wait();
  }

  auto SearchShared::send_go(time::TimePoint start_time, const SearchLimit& limits, const Game& g) -> void {
    search_start_time = start_time;
    search_main_limits = limits;
    search_game = &g;
    engine_message = EngineMessage::go;
    idle_barrier.arrive_and_wait();
    started_barrier.arrive_and_wait();
    search_game = nullptr;
  }

  auto SearchShared::stop() -> void {
    stopping = true;
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::reset() -> void {
    m_sd.reset();
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::launch() -> void {
    rose_assert(!m_thread.joinable());
    m_thread = std::jthread([this] {
      this->thread_main();
    });
  }

  auto calc_time(const SearchLimit& limits, Color stm) -> std::tuple<time::Duration, time::Duration> {
    constexpr time::Milliseconds margin_ms {100};
    constexpr time::Milliseconds zero_ms {0};

    const auto remaining_time = stm == Color::white ? limits.wtime : limits.btime;
    const auto increment = stm == Color::white ? limits.winc : limits.binc;

    const time::Milliseconds remaining_time_ms {remaining_time.value_or(0)};
    const time::Milliseconds increment_ms {increment.value_or(0)};
    const int movestogo = limits.movestogo.value_or(20);

    time::Milliseconds safe_remaining_ms = std::max(remaining_time_ms - margin_ms, zero_ms);

    if (limits.movetime) [[unlikely]] {
      const time::Milliseconds movetime_ms {*limits.movetime};
      if (!remaining_time && !increment)
        return {movetime_ms, movetime_ms};
      safe_remaining_ms = std::min(safe_remaining_ms, movetime_ms);
    }

    const time::Milliseconds hard_limit = std::min(safe_remaining_ms / movestogo * 7 + increment_ms / 3, safe_remaining_ms);
    const time::Milliseconds soft_limit = std::min(safe_remaining_ms / movestogo + increment_ms / 3, safe_remaining_ms);

    return {hard_limit, soft_limit};
  }

  auto calc_ctrl(time::TimePoint start_time, const SearchLimit& limits, Color stm) -> controls::Any {
    if (limits.has_time && !limits.has_other) {
      controls::Time ctrl;

      ctrl.start_time = start_time;
      std::tie(ctrl.hard_time, ctrl.soft_time) = calc_time(limits, stm);

      return ctrl;
    } else if (limits.has_time || limits.has_other) {
      controls::All ctrl;

      ctrl.start_time = start_time;
      if (limits.has_time)
        std::tie(ctrl.hard_time, ctrl.soft_time) = calc_time(limits, stm);
      ctrl.hard_nodes = limits.hard_nodes;
      ctrl.soft_nodes = limits.soft_nodes;
      ctrl.depth = limits.depth;

      return ctrl;
    } else {
      return controls::None {start_time};
    }
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::thread_main() -> void {
    while (true) {
      m_shared.idle_barrier.arrive_and_wait();

      switch (m_shared.engine_message) {
      case EngineMessage::ping:
        m_shared.started_barrier.arrive_and_wait();
        break;

      case EngineMessage::quit:
        return;

      case EngineMessage::go: {
        const Game& g = *m_shared.search_game;

        m_root = g.position();
        m_move_stack = g.move_stack();
        m_hash_stack = g.hash_stack();
        m_hash_waterline = std::max<usize>(1, m_hash_stack.size()) - 1;

        const auto ctrl = is_main_thread() ? calc_ctrl(m_shared.search_start_time, m_shared.search_main_limits, m_root.stm()) :
                                             controls::None {.start_time = m_shared.search_start_time};

        m_shared.stopping = false;
        stats().reset();

        (void)m_shared.started_barrier.arrive();

        std::visit(
          [this](const auto& ctrl) {
            this->search_root(ctrl);
          },
          ctrl);
      } break;
      }
    }
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::emergency_move(Line& pv) -> Score {
    // If we didn't complete depth 1 we have to scrounge up a move.

    // Try the TT first
    const auto tte = tt_load(m_root, 0);
    if (m_root.is_legal(tte.move)) {
      pv.write(tte.move);
      return tte.score;
    }

    // Otherwise, rely on our move ordering to pick a move.
    MovePicker moves {m_sd, m_root, &m_search_stack[search_stack_offset], Move::none()};
    pv.write(moves.next());
    return 0;
  }

  template<eval::concepts::State Evaluation>
  template<typename Controls>
  auto Search<Evaluation>::search_root(const Controls& ctrl) -> void {
    Line last_pv {};
    Score last_score = score::none;
    i32 last_depth = -1;

    const auto print_info = [&, this]() {
      m_shared.output->info(EngineOutput::Info {
        .depth = last_depth,
        .score = last_score,
        .time = ctrl.elapsed(),
        .nodes = m_shared.total_nodes(),
        .pv = last_pv,
      });
    };

    m_evaluation.reset(m_root);

    for (i32 depth = 1; depth < max_depth; depth++) {
      Line pv {};
      Score alpha = -score::infinity;
      Score beta = score::infinity;
      Score delta = 25;
      Score score = score::none;

      if (depth >= 4) {
        alpha = last_score - delta;
        beta = last_score + delta;
      }

      i32 aspiration_reduction = 0;

      while (true) {
        m_search_stack = {};
        pv.clear();
        m_nmr_ply = std::nullopt;

        const i32 aspiration_depth = std::max(1, depth - aspiration_reduction);
        SearchStack* ss = &m_search_stack[search_stack_offset];
        score = search<NodeType::pv, true>(ctrl, m_root, pv, alpha, beta, ss, 0, aspiration_depth);

        if (score <= alpha) {
          aspiration_reduction = 0;
          alpha = std::max(score - delta, -score::infinity);
        } else if (score >= beta) {
          aspiration_reduction = std::min(aspiration_reduction + 1, 3);
          beta = std::min(score + delta, score::infinity);
        } else {
          break;
        }

        delta += delta;

        if (m_shared.stopping)
          break;
      }

      if (m_shared.stopping)
        break;

      last_score = score;
      last_pv = pv;
      last_depth = depth;

      if (is_main_thread()) {
        if (ctrl.check_soft_termination(stats(), depth))
          break;
        print_info();
      }
    }

    m_shared.stop();

    if (is_main_thread()) {
      if (last_pv.pv.empty()) {
        last_score = emergency_move(last_pv);
        last_depth = 0;
      }

      print_info();
      m_shared.output->bestmove(last_pv.pv.empty() ? Move::none() : last_pv.pv[0]);
    }
  }

  template<eval::concepts::State Evaluation>
  template<NodeType expected, bool is_root, typename Controls>
  auto
    Search<Evaluation>::search(const Controls& ctrl, const Position& position, Line& pv, Score alpha, Score beta, SearchStack* ss, i32 ply, i32 depth)
      -> Score {
    if (depth <= 0)
      return qsearch<expected>(ctrl, position, pv, alpha, beta, ss, ply);

    stats().nodes.fetch_add(1, std::memory_order_relaxed);
    if (!is_root && is_main_thread() && ctrl.check_hard_termination(stats())) [[unlikely]] {
      m_shared.stop();
      return 0;
    }

    // Repetition Detection
    if (!is_root) {
      if (const auto score = position.is_fifty_move_draw(ply))
        return *score;

      if (position.is_repetition(m_hash_stack, m_hash_waterline))
        return 0;
    }

    if (ply >= max_depth)
      return eval(position);

    // Mate distance pruning
    if (!is_root) {
      alpha = std::max(alpha, score::mated(ply));
      beta = std::min(beta, score::mating(ply + 1));
      if (alpha >= beta)
        return alpha;
    }

    const bool excluded = ss->excluded.is_some();
    const bool is_in_check = position.is_in_check();
    const Color stm = position.stm();
    const Bitboard enemy_threatened = position.attack_table(!stm).bitboard_any();

    const tt::LookupResult tte = tt_load(position, ply);

    // Transposition Table Cutoffs
    if (expected != NodeType::pv && !excluded && tte.is_some() && tte.depth >= depth && [&] {
          switch (tte.bound.raw) {
          case NodeType::none:
            return false;
          case NodeType::cut:
            return tte.score >= beta;
          case NodeType::pv:
            return true;
          case NodeType::all:
            return tte.score <= alpha;
          }
        }()) {
      return tte.score;
    }

    const i32 static_eval = ss->static_eval != score::none ? ss->static_eval : is_in_check ? score::none : eval(position);
    ss->static_eval = static_eval;

    const bool improving = is_in_check                       ? false :
                           ss[-2].static_eval != score::none ? static_eval > ss[-2].static_eval :
                           ss[-4].static_eval != score::none ? static_eval > ss[-4].static_eval :
                                                               false;

    if (expected != NodeType::pv && !is_in_check && !excluded) {
      // Reverse Futility Pruning
      if (depth <= 15 && static_eval - 64 * depth - 4 * depth * depth >= beta) {
        return static_eval;
      }

      // Razoring
      if (depth <= 4 && static_eval + 600 * depth < alpha) {
        const Score razor_score = qsearch<expected.narrow()>(ctrl, position, pv, alpha, beta, ss, ply);
        if (razor_score <= alpha) {
          return razor_score;
        }
      }

      // Null move reductions
      if (depth >= 4 && m_nmr_ply != ply && ss[-1].move.is_some() && static_eval >= beta) {
        const i32 reduction = 4 + depth / 3;

        const Position null_position = make_null_move(ss, position);
        ss[1].static_eval = score::none;
        const Score null_score = -search<NodeType::all>(ctrl, null_position, pv, -beta, -beta + 1, ss + 1, ply + 1, depth - reduction);
        unmake_move(ss);

        if (m_shared.stopping)
          return 0;

        if (null_score >= beta) {
          if (m_nmr_ply.has_value()) {
            return null_score;
          } else {
            m_nmr_ply = ply;
            const Score score = search<expected>(ctrl, position, pv, alpha, beta, ss, ply, depth / 2);
            m_nmr_ply = std::nullopt;
            if (score >= beta)
              return score;
          }
        }
      }
    }

    MovePicker moves {m_sd, position, ss, tte.move};

    MoveList fail_low_quiets;
    MoveList fail_low_noisies;

    Score best_score = score::none;
    Move best_move = Move::none();
    NodeType actual_node_type = NodeType::all;
    u32 searched_moves = 0;

    for (Move mv = moves.next(); mv.is_some(); mv = moves.next()) {
      if (mv == ss->excluded)
        continue;

      const i32 history = [&] {
        const PieceType ptype = position.ptype_at(mv.from());

        i32 history = 0;
        if (!mv.is_noisy()) {
          history += m_sd.quiet_history.get(stm, enemy_threatened, mv);
          for (i32 i : {1, 2})
            if (ss[-i].conthist)
              history += ss[-i].conthist->get(stm, ptype, mv);
        }
        return history;
      }();

      if (!score::is_loss(best_score) && !is_in_check && !is_root) {
        // Late Move Pruning
        if (!mv.is_noisy() && searched_moves >= (4 + depth * depth) / (2 - improving)) {
          moves.skip_quiet();
          continue;
        }

        // Futility Pruning
        if (!mv.is_noisy() && depth <= 6 && std::abs(alpha) < 2000 && static_eval + 256 + depth * 100 <= alpha) {
          moves.skip_quiet();
          continue;
        }

        // History Pruning
        if (!mv.is_noisy() && depth <= 4 && history < -1024 * depth * depth) {
          moves.skip_quiet();
          continue;
        }

        // Quiet SEE Pruning
        if (!mv.is_noisy() && depth <= 11 && !see::see(position, mv, 32 - 48 * depth - 32 * history / 1024)) {
          continue;
        }

        // Noisy SEE Pruning
        if (mv.is_noisy() && depth <= 11 && !see::see(position, mv, -64 * depth)) {
          continue;
        }
      }

      i32 extension = 0;
      // Singular Extensions
      if (!is_root && depth >= 7 && mv == tte.move && !excluded && tte.depth >= depth - 3 && tte.bound.is_pv_or_cut()) {
        const Score singular_beta = std::max(score::min_score, tte.score - 2 * depth);
        const i32 singular_depth = depth / 2;

        ss->excluded = mv;
        const Score singular_score = search<expected.narrow()>(ctrl, position, pv, singular_beta - 1, singular_beta, ss, ply, singular_depth);
        ss->excluded = Move::none();

        // Multicut
        if (singular_score >= singular_beta && singular_beta >= beta) {
          return singular_beta;
        }

        if (singular_score < singular_beta) {
          // Single extension
          extension = 1;
          // Double extension
          extension += expected != NodeType::pv && singular_score <= singular_beta - 20;
          // Triple extension
          extension += expected != NodeType::pv && singular_score <= singular_beta - 120;
        }
        // Negative extension
        else if (expected == NodeType::cut) {
          extension = -2;
        } else if (tte.score <= alpha) {
          extension = -1;
        }
      }
      // Low Depth Singular Extensions
      else if (depth <= 6 && !is_in_check && expected == NodeType::cut && tte.bound.is_pv_or_cut() && tte.score >= std::max(beta, static_eval) + 64) {
        extension = 1;
      }

      searched_moves++;

      const Position child_position = make_move(ss, position, mv);
      ss[1].static_eval = score::none;
      rose_defer {
        unmake_move(ss);
      };

      const i32 new_depth = depth + extension - 1;
      Line child_pv {};
      Score score = score::none;

      // Late Move Reductions
      if (depth >= 3 && searched_moves > 2) {
        const i32 log2_depth = std::bit_width(static_cast<u32>(depth)) - 1;
        const i32 log2_searched_moves = std::bit_width(static_cast<u32>(searched_moves)) - 1;

        i32 reduction;

        if (mv.is_noisy()) {
          reduction = 1024 + 192 * log2_depth * log2_searched_moves;
        } else {
          reduction = 2176 + 256 * log2_depth * log2_searched_moves;
        }
        reduction -= 1024 * (expected == NodeType::pv);
        reduction -= 128 * history / 1024;
        reduction += 1024 * (expected == NodeType::cut);
        reduction -= 768 * child_position.is_in_check();

        const i32 lmr_depth = std::clamp(new_depth - reduction / 1024, 0, new_depth);

        score = -search<expected.next()>(ctrl, child_position, child_pv, -alpha - 1, -alpha, ss + 1, ply + 1, lmr_depth);

        if (score > alpha && lmr_depth < new_depth) {
          i32 research_depth = new_depth;
          if (!is_root) {
            research_depth += score > best_score + 64;
          }

          score = -search<expected.next()>(ctrl, child_position, child_pv, -alpha - 1, -alpha, ss + 1, ply + 1, research_depth);

          // Post-LMR continuation history update
          if (!mv.is_noisy() && (score <= alpha || score >= beta)) {
            const i32 cont_bonus = std::min(150 * depth - 75, 1536);
            const i32 cont_malus = std::min(75 * depth - 30, 1024);

            for (i32 i : conthists_indexes)
              if (ss[-i].conthist)
                ss[-i].conthist->update(stm, position.place_at(mv.from()).ptype(), mv, score <= alpha ? -cont_malus : cont_bonus);
          }
        }
      }
      // PVS Scout Search
      else if (expected != NodeType::pv || searched_moves > 1) {
        score = -search<expected.next()>(ctrl, child_position, child_pv, -alpha - 1, -alpha, ss + 1, ply + 1, new_depth);
      }
      // PVS Full Window Search
      if (expected == NodeType::pv && (searched_moves == 1 || score > alpha)) {
        score = -search<NodeType::pv>(ctrl, child_position, child_pv, -beta, -alpha, ss + 1, ply + 1, new_depth);
      }

      if (m_shared.stopping)
        return 0;

      if (score > best_score) {
        best_score = score;

        if (score > alpha) {
          actual_node_type = NodeType::pv;
          alpha = score;
          best_move = mv;

          if constexpr (expected == NodeType::pv)
            pv.write(mv, std::move(child_pv));

          if (score >= beta) {
            actual_node_type = NodeType::cut;
            break;
          }
        }
      }

      if (mv != best_move) {
        if (mv.is_noisy()) {
          fail_low_noisies.push_back(mv);
        } else {
          fail_low_quiets.push_back(mv);
        }
      }
    }

    if (best_score == score::none) {
      if (excluded)
        return score::min_score;
      return position.is_in_check() ? score::mated(ply) : 0;
    }

    if (best_move.is_some()) {
      const i32 noisy_bonus = std::min(150 * depth - 75, 1536);
      const i32 noisy_malus = std::min(75 * depth - 30, 1024);

      const i32 quiet_bonus = std::min(150 * depth - 75, 1536);
      const i32 quiet_malus = std::min(75 * depth - 30, 1024);

      const i32 cont_bonus = std::min(150 * depth - 75, 1536);
      const i32 cont_malus = std::min(75 * depth - 30, 1024);

      if (best_move.is_noisy()) {
        m_sd.noisy_history.update(stm, position.ptype_at(best_move.from()), best_move, noisy_bonus);
        for (const Move noisy : fail_low_noisies) {
          m_sd.noisy_history.update(stm, position.ptype_at(noisy.from()), noisy, -noisy_malus);
        }
      } else {
        m_sd.quiet_history.update(stm, enemy_threatened, best_move, quiet_bonus);
        for (i32 i : conthists_indexes)
          if (ss[-i].conthist)
            ss[-i].conthist->update(stm, position.place_at(best_move.from()).ptype(), best_move, cont_bonus);
        for (const Move quiet : fail_low_quiets) {
          m_sd.quiet_history.update(stm, enemy_threatened, quiet, -quiet_malus);
          for (i32 i : conthists_indexes)
            if (ss[-i].conthist)
              ss[-i].conthist->update(stm, position.place_at(quiet.from()).ptype(), quiet, -cont_malus);
        }
      }
    }

    if (!excluded) {
      tt_store(position,
               ply,
               tt::LookupResult {
                 .depth = depth,
                 .bound = actual_node_type,
                 .score = best_score,
                 .move = best_move,
               });
    }

    return best_score;
  }

  template<eval::concepts::State Evaluation>
  template<NodeType leaf_expected, typename Controls>
  auto Search<Evaluation>::qsearch(const Controls& ctrl, const Position& position, Line& pv, Score alpha, Score beta, SearchStack* ss, i32 ply)
    -> Score {
    stats().nodes.fetch_add(1, std::memory_order_relaxed);
    if (is_main_thread() && ctrl.check_hard_termination(stats())) [[unlikely]] {
      m_shared.stop();
      return 0;
    }

    // Repetition Detection
    {
      if (const auto score = position.is_fifty_move_draw(ply))
        return *score;

      if (position.is_repetition(m_hash_stack, m_hash_waterline))
        return 0;
    }

    if (ply >= max_depth)
      return eval(position);

    // Mate distance pruning
    {
      alpha = std::max(alpha, score::mated(ply));
      beta = std::min(beta, score::mating(ply + 1));
      if (alpha >= beta)
        return alpha;
    }

    const bool is_in_check = position.is_in_check();

    const tt::LookupResult tte = tt_load(position, ply);

    // Transposition Table Cutoffs
    if (leaf_expected != NodeType::pv && tte.is_some() && [&] {
          switch (tte.bound.raw) {
          case NodeType::none:
            return false;
          case NodeType::cut:
            return tte.score >= beta;
          case NodeType::pv:
            return true;
          case NodeType::all:
            return tte.score <= alpha;
          }
        }()) {
      return tte.score;
    }

    const Score static_eval = ss->static_eval != score::none ? ss->static_eval : is_in_check ? score::none : eval(position);
    ss->static_eval = static_eval;

    Score best_score = is_in_check ? score::mated(ply) : static_eval;

    // Standpat
    if (best_score >= beta) {
      return best_score;
    }
    alpha = std::max(alpha, best_score);

    MovePicker moves {m_sd, position, ss, Move::none()};
    moves.skip_quiet();

    Move best_move = Move::none();
    NodeType actual_node_type = NodeType::all;

    for (Move mv = moves.next(); mv.is_some(); mv = moves.next()) {
      if (!score::is_loss(best_score) && !is_in_check) {
        // QS SEE Pruning
        if (!see::see(position, mv, 0))
          continue;

        // QS Futility Pruning
        const Score futility = static_eval + 175;
        if (futility <= alpha && !see::see(position, mv, 1)) {
          best_score = std::max(best_score, futility);
          continue;
        }
      }

      const Position child_position = make_move(ss, position, mv);
      ss[1].static_eval = score::none;
      rose_defer {
        unmake_move(ss);
      };

      Line child_pv {};
      const Score score = -qsearch<leaf_expected>(ctrl, child_position, child_pv, -beta, -alpha, ss + 1, ply + 1);

      if (m_shared.stopping)
        return 0;

      if (score > best_score) {
        best_score = score;

        if (score > alpha) {
          actual_node_type = NodeType::pv;
          alpha = score;
          best_move = mv;
          if constexpr (leaf_expected == NodeType::pv)
            pv.write(mv, std::move(child_pv));

          if (score >= beta) {
            actual_node_type = NodeType::cut;
            break;
          }
        }
      }
    }

    return best_score;
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::eval(const Position& position) -> Score {
    return m_evaluation.evaluate(position);
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::tt_load(const Position& position, i32 ply) -> tt::LookupResult {
    return m_shared.transposition_table.load(position.hash(), ply);
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::tt_store(const Position& position, i32 ply, tt::LookupResult lr) -> void {
    m_shared.transposition_table.store(position.hash(), ply, lr);
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::make_move(SearchStack* ss, const Position& position, Move mv) -> Position {
    m_evaluation.push();
    const Position child_position = position.move(mv, m_evaluation.observer());
    m_hash_stack.push_back(child_position.hash());
    ss->move = mv;
    ss->conthist = m_sd.continuation_history.get_subtable(!child_position.stm(), child_position.place_at(mv.to()).ptype(), mv);
    return child_position;
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::make_null_move(SearchStack* ss, const Position& position) -> Position {
    m_evaluation.push();
    const Position child_position = position.null_move();
    m_hash_stack.push_back(child_position.hash());
    ss->move = Move::none();
    ss->conthist = nullptr;
    return child_position;
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::unmake_move(SearchStack* ss) -> void {
    m_evaluation.pop();
    m_hash_stack.pop_back();
    ss->move = Move::none();
    ss->conthist = nullptr;
  }

#define rose_search_template(e, T) template struct Search<eval::nnue::T::State>;
  rose_for_each_arch(rose_search_template);

}  // namespace rose
