#include "rose/tool/datagen/datagen.hpp"

#include "rose/common.hpp"
#include "rose/engine.hpp"
#include "rose/engine_output.hpp"
#include "rose/game.hpp"
#include "rose/movegen.hpp"
#include "rose/position.hpp"
#include "rose/score.hpp"
#include "rose/search.hpp"
#include "rose/util/assert.hpp"
#include "rose/util/defer.hpp"
#include "rose/util/time.hpp"
#include "rose/version.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <csignal>
#endif

namespace rose::tool::datagen {

  static std::atomic_bool g_stop = false;

  struct DatagenConfig {
    std::string base_filename;
    usize initial_move_count;
    int soft_nodes;
    int hard_nodes;
    usize thread_count;
    u64 root_seed;
    std::string timestamp;
  };

  struct DatagenState {
    std::atomic<u64> total_game_count = 0;
    std::atomic<u64> total_position_count = 0;
    time::TimePoint start_time;
  };

  static auto write_meta_file(const DatagenConfig& dgc) -> void;
  static auto thread_main(int thread_index, DatagenState& state, const DatagenConfig& dgc, u64 thread_seed) -> void;

  auto run(usize thread_count) -> void {
    fmt::print("# 🌹 Rose Datagen {}\n", rose::version::to_string());

    const auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
    const std::string timestamp = fmt::format("{:%Y%m%d-%H%M%S}", now);
    const std::string base_filename = fmt::format("datagen-{}-{}", timestamp, version::git_commit_desc);

    std::random_device rd;
    std::uniform_int_distribution<u64> seed_distrib;
    const u64 root_seed = seed_distrib(rd);

    fmt::print("# Output base filename: {}\n", base_filename);
    fmt::print("# Thread count: {}\n", thread_count);
    fmt::print("# Root seed: {:016x}\n", root_seed);

    std::fflush(stdout);

    DatagenConfig dgc {
      .base_filename = base_filename,
      .initial_move_count = 8,
      .soft_nodes = 7000,
      .hard_nodes = 10000000,
      .thread_count = thread_count,
      .root_seed = root_seed,
      .timestamp = timestamp,
    };

    DatagenState state {
      .start_time = time::Clock::now(),
    };

    write_meta_file(dgc);

#if _WIN32
    // Setup Ctrl+C handler
    if (!SetConsoleCtrlHandler(
          +[](DWORD) -> int {
            g_stop.store(true);
            return true;
          },
          true)) {
      fmt::print("SetConsoleCtrlHandler failed\n");
      std::exit(-1);
    }
#else
    // Setup signal handlers
    {
      struct sigaction action;
      sigemptyset(&action.sa_mask);
      sigaddset(&action.sa_mask, SIGINT);
      sigaddset(&action.sa_mask, SIGTERM);
      action.sa_flags = SA_RESTART;
      action.sa_handler = [](int) {
        g_stop.store(true);
      };
      if (sigaction(SIGINT, &action, nullptr) < 0) {
        fmt::print("sigaction(SIGINT) failed\n");
        std::exit(-1);
      }
      if (sigaction(SIGTERM, &action, nullptr) < 0) {
        fmt::print("sigaction(SIGTERM) failed\n");
        std::exit(-1);
      }
    }
#endif

    std::mt19937_64 per_thread_seed_gen {dgc.root_seed};
    std::vector<std::jthread> threads;
    for (int i = 0; i < thread_count; i++) {
      const u64 thread_seed = per_thread_seed_gen();
      threads.emplace_back(&thread_main, i, std::ref(state), dgc, thread_seed);
    }

    for (int i = 0; i < thread_count; i++) {
      threads[i].join();
    }

    fmt::print("\ndone.\n");
  }

  static auto write_meta_file(const DatagenConfig& dgc) -> void {
    const std::string filename = dgc.base_filename + ".txt";
    FILE* f = std::fopen(filename.c_str(), "wxb");
    if (!f) {
      fmt::print("unable to open output metafile {}\n", filename);
      std::exit(-1);
    }
    rose_defer {
      std::fclose(f);
    };

    fmt::print(f, "# Rose Datagen Metafile\n");
    fmt::print(f, "filename: {}\n", filename);
    fmt::print(f, "thread_count: {}\n", dgc.thread_count);
    fmt::print(f, "initial_move_count: {}\n", dgc.initial_move_count);
    fmt::print(f, "soft_nodes: {}\n", dgc.soft_nodes);
    fmt::print(f, "hard_nodes: {}\n", dgc.hard_nodes);
    fmt::print(f, "root_seed: {:016x}\n", dgc.root_seed);
    fmt::print(f, "timestamp: {}\n", dgc.timestamp);
    fmt::print(f, "version_string: {}\n", version::version_string);
    fmt::print(f, "git_commit_hash: {}\n", version::git_commit_hash);
    fmt::print(f, "git_commit_desc: {}\n", version::git_commit_desc);
  }

  enum class GameResult {
    black_win = 0,
    draw = 1,
    white_win = 2,
  };

  struct GameRecord {
    Game game;
    GameResult result;
    std::vector<Score> scores;

    auto write(FILE* f) const -> void;
  };

  struct OutputCapture : public EngineOutput {
    Score score;
    Move move;

    ~OutputCapture() = default;

    auto info(Info args) -> void override {
      score = args.score;
    }

    auto bestmove(Move mv) -> void override {
      move = mv;
    }
  };

  static auto do_search(const DatagenConfig& dgc, Engine& engine, const Game& game) -> std::tuple<Score, Move> {
    const std::shared_ptr<OutputCapture> output = std::make_shared<OutputCapture>();
    engine.set_output(output);
    engine.run_search(time::Clock::now(),
                      SearchLimit {
                        .has_other = true,
                        .hard_nodes = dgc.hard_nodes,
                        .soft_nodes = dgc.soft_nodes,
                      },
                      game);
    engine.wait();
    return {output->score, output->move};
  }

  template<typename Random>
  static auto play_game(const DatagenConfig& dgc, Random& rand, Engine& engine) -> GameRecord {
    // Play random opening
    const Position initial_position = [&]() -> Position {
retry:
      Position position = [&] {
        std::uniform_int_distribution<usize> variant_dist {0, 4};
        if (variant_dist(rand) == 0)
          return Position::startpos();

        std::uniform_int_distribution<usize> pos_dist {0, 960 * 960 - 1};
        return Position::dfrcstartpos(pos_dist(rand));
      }();

      for (usize i = 0; i < dgc.initial_move_count; ++i) {
        MoveList moves = generate_all_moves(position);
        if (moves.size() == 0)
          goto retry;

        std::uniform_int_distribution<usize> move_dist {0, moves.size() - 1};
        position = position.move(moves[move_dist(rand)]);
      }

      if (position.has_no_legal_moves_slow())
        goto retry;

      return position;
    }();

    GameRecord record;
    record.game.set_position(initial_position);

    // Play game
    while (true) {
      const auto [score, move] = do_search(dgc, engine, record.game);
      const bool is_white = record.game.position().stm() == Color::white;

      record.scores.push_back(is_white ? score : -score);  // White-relative score
      record.game.move(move);

      if (record.game.is_draw_slow()) {
        record.result = GameResult::draw;
        break;
      }
      if (record.game.position().has_no_legal_moves_slow()) {
        rose_assert(record.game.position().is_in_check());
        record.result = is_white ? GameResult::white_win : GameResult::black_win;
        break;
      }
    }

    return record;
  }

  static auto thread_main(int thread_index, DatagenState& state, const DatagenConfig& dgc, u64 thread_seed) -> void {
    const std::string filename = dgc.base_filename + "." + std::to_string(thread_index) + ".viriformat";
    FILE* f = std::fopen(filename.c_str(), "wxb");
    if (!f) {
      fmt::print("unable to open output file {}\n", filename);
      std::exit(-1);
    }
    rose_defer {
      std::fclose(f);
    };

    Engine engine;
    std::mt19937_64 rand {thread_seed};

    while (!g_stop) {
      const auto game_record = play_game(dgc, rand, engine);

      game_record.write(f);

      state.total_game_count++;
      state.total_position_count += game_record.game.move_stack().size();

      {
        const u64 game_count = state.total_game_count.load(std::memory_order::relaxed);
        const u64 position_count = state.total_position_count.load(std::memory_order::relaxed);

        const auto elapsed = time::cast<time::FloatSeconds>(time::Clock::now() - state.start_time);
        const f64 positions_per_second = static_cast<f64>(position_count) / elapsed.count();

        fmt::print("games: {}, positions: {}, {:.1f} positions/second\r", game_count, position_count, positions_per_second);
        std::fflush(stdout);
      }
    }
  }

  template<typename T>
  auto push(std::vector<u8>& output, T value) -> void {
    const char* ptr = reinterpret_cast<const char*>(&value);
    for (usize i = 0; i < sizeof(T); i++)
      output.push_back(static_cast<u8>(*ptr++));
  }

  auto GameRecord::write(FILE* f) const -> void {
    std::vector<u8> output;

    const Position initial_position = game.initial_position();
    const RookInfo rook_info = initial_position.rook_info();
    const Bitboard occupied = initial_position.board().occupied_bitboard();

    push<u64>(output, occupied.raw);

    std::array<u8, 32> pieces {};
    int i = 0;
    for (const Square sq : occupied) {
      const Place p = initial_position.place_at(sq);

      pieces[i] = [&] {
        switch (p.ptype().raw) {
        case PieceType::p:
          return 0;
        case PieceType::n:
          return 1;
        case PieceType::b:
          return 2;
        case PieceType::r:
          return rook_info.has(sq) ? 6 : 3;
        case PieceType::q:
          return 4;
        case PieceType::k:
          return 5;
        default:
          rose_assert(false);
          return 0;
        }
      }();

      if (p.color() == Color::black)
        pieces[i] |= 8;

      i++;
    }

    for (int i = 0; i < 16; i++) {
      push<u8>(output, pieces[i * 2] | (pieces[i * 2 + 1] << 4));
    }

    push<u8>(output, initial_position.stm().to_msb8() | (initial_position.enpassant().is_valid() ? initial_position.enpassant().raw : 64));
    push<u8>(output, initial_position.fifty_move_clock());
    push<u16>(output, initial_position.full_move_counter());
    push<u16>(output, 0);                       // Score
    push<u8>(output, static_cast<u8>(result));  // Game result
    push<u8>(output, 0);                        // Padding

    const auto moves = game.move_stack();
    rose_assert(moves.size() == scores.size());
    const usize move_count = moves.size();

    for (usize i = 0; i < move_count; i++) {
      const Move move = moves[i];
      const Score score = scores[i];

      u16 move_bits = 0;
      move_bits |= static_cast<u16>(move.from().raw);
      move_bits |= static_cast<u16>(move.to().raw) << 6;
      if (move.is_promo()) {
        move_bits |= [&] {
          switch (move.ptype().raw) {
          case PieceType::n:
            return 0 << 12;
          case PieceType::b:
            return 1 << 12;
          case PieceType::r:
            return 2 << 12;
          case PieceType::q:
            return 3 << 12;
          default:
            rose_assert(false);
            return 0;
          }
        }();
      }
      if (move.is_enpassant())
        move_bits |= 1 << 14;
      if (move.is_castle())
        move_bits |= 2 << 14;
      if (move.is_promo())
        move_bits |= 3 << 14;

      push<u16>(output, move_bits);
      push<i16>(output, static_cast<i16>(score));
    }

    push<u32>(output, 0);  // Terminator

    std::fwrite(output.data(), sizeof(u8), output.size(), f);
    std::fflush(f);
  }

}  // namespace rose::tool::datagen
