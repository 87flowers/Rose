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
#include "rose/tune.hpp"
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
    const int movestogo = limits.movestogo.value_or(16_z);

    time::Milliseconds safe_remaining_ms = std::max(remaining_time_ms - margin_ms, zero_ms);

    if (limits.movetime) [[unlikely]] {
      const time::Milliseconds movetime_ms {*limits.movetime};
      if (!remaining_time && !increment)
        return {movetime_ms, movetime_ms};
      safe_remaining_ms = std::min(safe_remaining_ms, movetime_ms);
    }

    const auto hard_limit = std::min<time::Milliseconds>(safe_remaining_ms / movestogo * 7_z + increment_ms / 3, safe_remaining_ms);
    const auto soft_limit = std::min<time::Milliseconds>(safe_remaining_ms / movestogo + increment_ms / 3, safe_remaining_ms);

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

        rose_assert(m_hash_stack.back() == m_root.calc_hashes_slow());

        const auto ctrl = is_main_thread() ? calc_ctrl(m_shared.search_start_time, m_shared.search_main_limits, m_root.stm()) :
                                             controls::None {.start_time = m_shared.search_start_time};

        m_shared.stopping = false;
        stats().reset();

        if (is_main_thread())
          m_shared.transposition_table.increment_age();

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
    const auto tte = tt_load();
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

    i32 pv_last_unstable = 0;
    i32 score_last_unstable = 0;

    for (i32 depth = 1; depth < max_depth; depth++) {
      Line pv {};
      Score alpha = -score::infinity;
      Score beta = score::infinity;
      Score delta = 17_z;
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
        m_iid_iteration = 0;

        const i32 aspiration_depth = std::max(1, depth - aspiration_reduction);
        SearchStack* ss = &m_search_stack[search_stack_offset];
        score = search<NodeType::pv, true>(ctrl, m_root, pv, alpha, beta, ss, 0, aspiration_depth);

        if (score <= alpha) {
          aspiration_reduction = 0;
          beta = (alpha + beta) / 2;
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

      if (last_pv.first_move() != pv.first_move())
        pv_last_unstable = depth;
      if (std::abs(last_score - score) >= 28_z)
        score_last_unstable = depth;

      last_score = score;
      last_pv = pv;
      last_depth = depth;

      if (is_main_thread()) {
        const i32 pv_stability = depth - pv_last_unstable;
        const i32 score_stability = depth - score_last_unstable;

        const f32 time_multiplier = std::clamp(1.0 - pv_stability * 0.05116902193882232_z, 0.592028611604442_z, 1.0855072225856215_z) *
                                    std::clamp(1.0 - score_stability * 0.04828987665300888_z, 0.667434450591335_z, 1.1385035397539525_z);

        if (ctrl.check_soft_termination(stats(), depth, time_multiplier))
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
      if (const auto score = draw::is_fifty_move_draw(position, ply))
        return *score;

      if (draw::is_repetition(position, m_hash_stack, m_hash_waterline))
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

    const tt::LookupResult tte = tt_load();
    Move hint_move = tte.move;

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

    const Score raw_static_eval = [&, this] {
      if (is_in_check)
        return score::none;
      if (ss->raw_static_eval != score::none)
        return ss->raw_static_eval;
      if (tte.raw_eval != score::none)
        return tte.raw_eval;
      return eval(position);
    }();
    ss->raw_static_eval = raw_static_eval;

    const auto [correction, static_eval] = [&, this] -> std::tuple<std::optional<i32>, Score> {
      if (is_in_check)
        return {std::nullopt, score::none};

      const i32 correction = eval_correction(position);
      const Score static_eval = correct_eval(position, raw_static_eval, correction);
      return {correction, static_eval};
    }();
    ss->static_eval = static_eval;

    const bool improving = is_in_check                       ? false :
                           ss[-2].static_eval != score::none ? static_eval > ss[-2].static_eval :
                           ss[-4].static_eval != score::none ? static_eval > ss[-4].static_eval :
                                                               false;

    if (expected != NodeType::pv && !is_in_check && !excluded) {
      // Hindsight extension
      if (depth <= 20 && ss[-1].reduction >= 4 && ss[-1].static_eval != score::none && static_eval <= -ss[-1].static_eval) {
        depth++;
      }

      // Reverse Futility Pruning
      if (depth <= 15 && static_eval - 59_z * depth - 4_z * depth * depth >= beta) {
        return static_eval;
      }

      // Razoring
      if (depth <= 4 && static_eval + 548_z * depth < alpha) {
        const Score razor_score = qsearch<expected.narrow()>(ctrl, position, pv, alpha, beta, ss, ply);
        if (razor_score <= alpha) {
          return razor_score;
        }
      }

      // Null move reductions
      if (depth >= 4 && m_nmr_ply != ply && ss[-1].move.is_some() && static_eval >= beta) {
        const i32 reduction = (4754_z + depth * 344_z) / 1024;

        const Position null_position = make_null_move(ss, position);
        const Score null_score = -search<expected.next()>(ctrl, null_position, pv, -beta, -beta + 1, ss + 1, ply + 1, depth - reduction);
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

    // Internal iterative deepening
    if (!is_root && expected == NodeType::pv && depth >= 8 && hint_move.is_none() && !excluded) {
      const i32 iid_depth = (794_z * depth - 1525_z) / 1024;

      m_iid_iteration++;
      search<NodeType::pv>(ctrl, position, pv, alpha, beta, ss, ply, iid_depth);
      m_iid_iteration--;

      const auto iid_tte = tt_load();
      hint_move = iid_tte.move;
      if (m_iid_iteration > 0 && iid_tte.depth >= depth)
        return iid_tte.score;
    }

    MovePicker moves {m_sd, position, ss, hint_move};

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
        if (!mv.is_noisy() && searched_moves >= i64 {4415_z + 953_z * depth * depth} * (526_z + 550_z * improving) / (1024 * 1024)) {
          moves.skip_quiet();
          continue;
        }

        // Futility Pruning
        if (!mv.is_noisy() && depth <= 6 && std::abs(alpha) < 2000 && static_eval + 267_z + depth * 104_z <= alpha) {
          moves.skip_quiet();
          continue;
        }

        // History Pruning
        if (!mv.is_noisy() && depth <= 4 && history < -1007_z * depth * depth) {
          moves.skip_quiet();
          continue;
        }

        // Quiet SEE Pruning
        if (!mv.is_noisy() && depth <= 11 && !see::see(position, mv, 32_z - 49_z * depth - 34_z * history / 1024)) {
          continue;
        }

        // Noisy SEE Pruning
        if (mv.is_noisy() && depth <= 11 && !see::see(position, mv, -73_z * depth)) {
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
          extension += expected != NodeType::pv && singular_score <= singular_beta - 21_z;
          // Triple extension
          extension += expected != NodeType::pv && singular_score <= singular_beta - 131_z;
        }
        // Negative extension
        else if (expected == NodeType::cut) {
          extension = -2;
        } else if (tte.score <= alpha) {
          extension = -1;
        }
      }

      searched_moves++;

      const Position child_position = make_move(ss, position, mv);
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
          reduction = 1021_z + 180_z * log2_depth * log2_searched_moves;
        } else {
          reduction = 2255_z + 214_z * log2_depth * log2_searched_moves;
        }
        reduction -= 970_z * (expected == NodeType::pv);
        reduction -= 132_z * history / 1024;
        reduction += 937_z * (expected == NodeType::cut);
        reduction -= 844_z * child_position.is_in_check();

        const i32 lmr_depth = std::min(std::max(new_depth - reduction / 1024, 0), new_depth) + (expected == NodeType::pv);

        ss->reduction = new_depth - lmr_depth;
        score = -search<expected.next()>(ctrl, child_position, child_pv, -alpha - 1, -alpha, ss + 1, ply + 1, lmr_depth);
        ss->reduction = 0;

        if (score > alpha && lmr_depth < new_depth) {
          i32 research_depth = new_depth;
          if (!is_root) {
            research_depth += score > best_score + 64;
          }

          score = -search<expected.next()>(ctrl, child_position, child_pv, -alpha - 1, -alpha, ss + 1, ply + 1, research_depth);

          // Post-LMR continuation history update
          if (!mv.is_noisy() && (score <= alpha || score >= beta)) {
            const i32 cont_bonus = std::min(163_z * depth - 74_z, 1529_z);
            const i32 cont_malus = std::min(78_z * depth - 29_z, 1073_z);

            for (i32 i : conthists_indexes)
              if (ss[-i].conthist)
                ss[-i].conthist->update(stm, position.ptype_at(mv.from()), mv, score <= alpha ? -cont_malus : cont_bonus);
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
      const i32 noisy_bonus = std::min(153_z * depth - 76_z, 1614_z);
      const i32 noisy_malus = std::min(65_z * depth - 28_z, 944_z);

      const i32 quiet_bonus = std::min(164_z * depth - 87_z, 1705_z);
      const i32 quiet_malus = std::min(83_z * depth - 28_z, 897_z);

      const i32 cont_bonus = std::min(137_z * depth - 74_z, 1463_z);
      const i32 cont_malus = std::min(98_z * depth - 34_z, 1123_z);

      if (best_move.is_noisy()) {
        m_sd.noisy_history.update(stm, position.ptype_at(best_move.from()), best_move, noisy_bonus);
        for (const Move noisy : fail_low_noisies) {
          m_sd.noisy_history.update(stm, position.ptype_at(noisy.from()), noisy, -noisy_malus);
        }
      } else {
        m_sd.quiet_history.update(stm, enemy_threatened, best_move, quiet_bonus);
        for (i32 i : conthists_indexes)
          if (ss[-i].conthist)
            ss[-i].conthist->update(stm, position.ptype_at(best_move.from()), best_move, cont_bonus);
        for (const Move quiet : fail_low_quiets) {
          m_sd.quiet_history.update(stm, enemy_threatened, quiet, -quiet_malus);
          for (i32 i : conthists_indexes)
            if (ss[-i].conthist)
              ss[-i].conthist->update(stm, position.ptype_at(quiet.from()), quiet, -cont_malus);
        }
      }
    }

    if (!excluded) {
      if (!is_in_check && !best_move.is_noisy() && [&] {
            switch (actual_node_type.raw) {
            case NodeType::pv:
              return true;
            case NodeType::all:
              return best_score < static_eval;
            case NodeType::cut:
              return best_score > static_eval;
            case NodeType::none:
              return false;
            }
          }()) {
        const i32 bonus = (best_score - static_eval) * depth / 4;
        m_sd.pawn_correction_history.update(stm, m_hash_stack.back().pawn, bonus);
        m_sd.non_pawn_correction_history[Color::white].update(stm, m_hash_stack.back().non_pawn[Color::white], bonus);
        m_sd.non_pawn_correction_history[Color::black].update(stm, m_hash_stack.back().non_pawn[Color::black], bonus);
      }

      tt_store(tt::LookupResult {
        .depth = depth,
        .bound = actual_node_type,
        .score = best_score,
        .raw_eval = raw_static_eval,
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
      if (const auto score = draw::is_fifty_move_draw(position, ply))
        return *score;

      if (draw::is_repetition(position, m_hash_stack, m_hash_waterline))
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
    const Color stm = position.stm();

    const tt::LookupResult tte = tt_load();

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

    const Score raw_static_eval = [&, this] {
      if (is_in_check)
        return score::none;
      if (ss->raw_static_eval != score::none)
        return ss->raw_static_eval;
      if (tte.raw_eval != score::none)
        return tte.raw_eval;
      return eval(position);
    }();
    ss->raw_static_eval = raw_static_eval;

    const auto [correction, static_eval] = [&, this] -> std::tuple<std::optional<i32>, Score> {
      if (is_in_check)
        return {std::nullopt, score::none};

      const i32 correction = eval_correction(position);
      const Score static_eval = correct_eval(position, raw_static_eval, correction);
      return {correction, static_eval};
    }();
    ss->static_eval = static_eval;

    Score best_score = is_in_check ? score::mated(ply) : static_eval;

    // Standpat
    if (best_score >= beta) {
      return best_score;
    }
    alpha = std::max(alpha, best_score);

    MovePicker moves {m_sd, position, ss, Move::none()};
    if (!is_in_check)
      moves.skip_quiet();

    Move best_move = Move::none();
    NodeType actual_node_type = NodeType::all;

    for (Move mv = moves.next(); mv.is_some(); mv = moves.next()) {
      if (!score::is_loss(best_score) && !is_in_check) {
        // QS SEE Pruning
        if (!see::see(position, mv, 0))
          continue;

        // QS Futility Pruning
        const Score futility = static_eval + 167_z;
        if (futility <= alpha && !see::see(position, mv, 1)) {
          best_score = std::max(best_score, futility);
          continue;
        }
      }

      const Position child_position = make_move(ss, position, mv);
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

      // Limit evasions
      if (is_in_check && !score::is_loss(best_score))
        moves.skip_quiet();
    }

    tt_store(tt::LookupResult {
      .depth = 0,
      .bound = actual_node_type,
      .score = best_score,
      .raw_eval = raw_static_eval,
      .move = best_move,
    });

    return best_score;
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::eval(const Position& position) -> Score {
    return m_evaluation.evaluate(position);
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::eval_correction(const Position& position) -> i32 {
    return m_sd.pawn_correction_history.get(position.stm(), m_hash_stack.back().pawn) +
           m_sd.non_pawn_correction_history[Color::white].get(position.stm(), m_hash_stack.back().non_pawn[Color::white]) +
           m_sd.non_pawn_correction_history[Color::black].get(position.stm(), m_hash_stack.back().non_pawn[Color::black]);
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::correct_eval(const Position& position, Score raw_static_eval, i32 correction) -> Score {
    return score::clamp_normal(raw_static_eval + correction / 64);
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::tt_load() -> tt::LookupResult {
    rose_assert(m_hash_stack.size() > m_hash_waterline);
    const i32 ply = static_cast<i32>(m_hash_stack.size() - 1 - m_hash_waterline);
    return m_shared.transposition_table.load(m_hash_stack.back().full, ply);
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::tt_store(tt::LookupResult lr) -> void {
    rose_assert(m_hash_stack.size() > m_hash_waterline);
    const i32 ply = static_cast<i32>(m_hash_stack.size() - 1 - m_hash_waterline);
    m_shared.transposition_table.store(m_hash_stack.back().full, ply, lr);
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::make_move(SearchStack* ss, const Position& position, Move mv) -> Position {
    m_evaluation.push();
    m_hash_stack.push_back(position.hashes_after(m_hash_stack.back(), mv));
    const Position child_position = position.move(mv, m_evaluation.observer());
    ss->move = mv;
    ss->conthist = m_sd.continuation_history.get_subtable(!child_position.stm(), child_position.ptype_at(mv.to()), mv);
    ss[1].raw_static_eval = score::none;
    ss[1].static_eval = score::none;
    return child_position;
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::make_null_move(SearchStack* ss, const Position& position) -> Position {
    m_evaluation.push();
    m_hash_stack.push_back(position.hashes_after_null_move(m_hash_stack.back()));
    const Position child_position = position.null_move();
    ss->move = Move::none();
    ss->conthist = nullptr;
    ss[1].raw_static_eval = score::none;
    ss[1].static_eval = score::none;
    return child_position;
  }

  template<eval::concepts::State Evaluation>
  auto Search<Evaluation>::unmake_move(SearchStack* ss) -> void {
    m_evaluation.pop();
    m_hash_stack.pop_back();
    ss->move = Move::none();
    ss->conthist = nullptr;
    ss[1].raw_static_eval = score::none;
    ss[1].static_eval = score::none;
  }

#define rose_search_template(e, T) template struct Search<eval::nnue::T::State>;
  rose_for_each_arch(rose_search_template);

}  // namespace rose
