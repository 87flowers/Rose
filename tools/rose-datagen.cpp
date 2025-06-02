#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <print>
#include <random>
#include <signal.h>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>

#include "rose/engine.h"
#include "rose/eval/eval.h"
#include "rose/game.h"
#include "rose/move.h"
#include "rose/movegen.h"
#include "rose/tool/game_result.h"
#include "rose/util/defer.h"
#include "rose/util/string.h"

namespace rose::tool::datagen {

  std::atomic<bool> g_stop = false;

  struct DatagenConfig {
    usize initial_move_count;
    int soft_nodes;
    int hard_nodes;
    int thread_count;
    u64 root_seed;
    std::string timestamp;
  };

  struct DatagenState {
    std::mutex mutex{};
    std::FILE *file = nullptr;
    u64 total_game_count = 0;
    u64 total_position_count = 0;
    time::TimePoint start_time;
  };

  static auto fwrite(FILE *file, const std::vector<u8> &data) -> void { std::fwrite(data.data(), sizeof(u8), data.size(), file); }
  template <typename T> static auto fwrite(FILE *file, T value) -> void { std::fwrite(&value, sizeof(T), 1, file); }

  static auto pushBytes(std::vector<u8> &output, const void *ptr_void, usize size) -> void {
    const char *ptr = reinterpret_cast<const char *>(ptr_void);
    for (usize i = 0; i < size; i++)
      output.push_back(static_cast<u8>(*ptr++));
  }
  template <typename T> static auto push(std::vector<u8> &output, T value) -> void { pushBytes(output, &value, sizeof(T)); }
  static auto pushString(std::vector<u8> &output, std::string_view str) -> void {
    push<u16>(output, str.size());
    pushBytes(output, str.data(), str.size());
  }

  static auto writeFileHeader(FILE *file, const DatagenConfig &dgc) -> void {
    fwrite(file, {'R', 'P', 'G', '1'});

    std::vector<u8> header;
    push<u16>(header, 1);
    push<u16>(header, static_cast<u8>(dgc.thread_count));
    push<u8>(header, 0);
    push<u8>(header, dgc.initial_move_count);
    push<u32>(header, dgc.soft_nodes);
    push<u32>(header, dgc.hard_nodes);
    push<u64>(header, dgc.root_seed);
    pushString(header, dgc.timestamp);
    pushString(header, ROSE_VERSION);
    pushString(header, ROSE_GIT_COMMIT_DESC);
    pushString(header, ROSE_GIT_COMMIT_HASH);
    push<u32>(header, 0);

    while (header.size() % 2 != 0)
      header.push_back(0);

    fwrite<u16>(file, header.size());
    fwrite(file, header);
  }

  template <typename Random> static auto makeRandomMove(Random &rand, Game &game) -> bool {
    MoveList moves;
    const PrecompMoveGenInfo movegen_precomp{game.position()};
    MoveGen movegen{game.position(), movegen_precomp};
    movegen.generateMoves(moves);

    if (moves.size() == 0)
      return false;

    std::uniform_int_distribution<usize> d{0, moves.size() - 1};
    const usize i = d(rand);
    game.move(moves[i]);
    return true;
  }

  template <typename Random> static auto playRandomMoves(Random &rand, Game &game, usize initial_move_count) -> void {
    game.reset();
    for (usize i = 0; i < initial_move_count; i++) {
      const bool valid = makeRandomMove(rand, game);
      if (!valid)
        return playRandomMoves(rand, game, initial_move_count);
    }
  }

  struct OutputCapture : public EngineOutput {
    i32 score = 0;
    Move move = Move::none();
    ~OutputCapture() = default;
    auto info(Info args) -> void override { score = args.score; }
    auto bestmove(Move m) -> void override { move = m; }
  };

  static auto doSearch(const DatagenConfig &dgc, Engine &engine, Game &game) -> std::tuple<i32, Move> {
    const std::shared_ptr<OutputCapture> output = std::make_shared<OutputCapture>();
    engine.setOutput(output);
    engine.setGame(game);
    engine.runSearch(time::Clock::now(), SearchLimit{
                                             .has_other = true,
                                             .hard_nodes = dgc.hard_nodes,
                                             .soft_nodes = dgc.soft_nodes,
                                         });
    engine.isReady();
    return {output->score, output->move};
  }

  template <typename Random> static auto playGame(const DatagenConfig &dgc, Random &rand, Engine &engine) -> std::tuple<std::vector<u8>, u64> {
    std::vector<u8> output;
    Game game;

    // Play random opening
    {
      playRandomMoves(rand, game, dgc.initial_move_count);
      const std::vector<Move> opening_moves = game.moveStack();
      push<u16>(output, 0xA000 + opening_moves.size());
      for (const Move m : opening_moves)
        push<u16>(output, m.raw);
    }

    GameResult result;
    u64 moves_played = 0;

    // Play game
    while (true) {
      const auto [score, move] = doSearch(dgc, engine, game);
      const bool is_white = game.position().activeColor() == Color::white;

      if (score == eval::mated(0)) {
        rose_assert(move == Move::none());
        result = is_white ? GameResult::black_win : GameResult::white_win;
        break;
      }
      if (game.isDraw()) {
        rose_assert(score == 0);
        result = GameResult::draw;
        break;
      }

      rose_assert(move != Move::none());
      push<u16>(output, move.raw);
      push<i16>(output, narrow_cast<i16>(is_white ? score : -score));
      moves_played++;

      game.move(move);
    }

    // Terminator
    push<u16>(output, 0);
    push<u16>(output, 0);

    // Game result
    output[1] = 0xA0 + std::to_underlying(result);

    return {output, moves_played};
  }

  static auto threadMain(DatagenState &state, const DatagenConfig &dgc, u64 thread_seed) -> void {
    Engine engine;
    std::mt19937_64 rand{thread_seed};

    while (!g_stop.load()) {
      const auto [game_record, position_count] = playGame(dgc, rand, engine);

      {
        // std::unique_lock _{state.mutex};
        fwrite(state.file, game_record);

        state.total_game_count++;
        state.total_position_count += position_count;

        const auto elapsed = time::cast<time::FloatSeconds>(time::Clock::now() - state.start_time);
        const f64 positions_per_second = static_cast<f64>(state.total_position_count) / elapsed.count();

        std::print("games: {}, positions: {}, {:.1f} positions/second\r", state.total_game_count, state.total_position_count, positions_per_second);
        std::fflush(stdout);
      }
    }
  }

  static auto run(int thread_count) -> void {
    std::print("# 🌹 Rose Datagen {}-{}\n", ROSE_VERSION, ROSE_GIT_COMMIT_DESC);

    DatagenConfig dgc;
    DatagenState state;

    dgc.initial_move_count = 8;
    dgc.soft_nodes = 5000;
    dgc.hard_nodes = 500000;
    dgc.thread_count = thread_count;

    state.start_time = time::Clock::now();

    const auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::utc_clock::now());
    dgc.timestamp = std::format("{:%Y%m%d-%H%M%S}", now);
    const std::string filename = std::format("datagen-{}-{}.rpg", dgc.timestamp, ROSE_GIT_COMMIT_DESC);
    std::print("# Output filename: {}\n", filename);

    std::print("# Threads: {}\n", dgc.thread_count);

    std::random_device rd;
    std::uniform_int_distribution<u64> seed_distrib;
    dgc.root_seed = seed_distrib(rd);
    std::print("# Seed: {:016x}\n", dgc.root_seed);

    std::fflush(stdout);

    state.file = std::fopen(filename.c_str(), "wxb");
    if (!state.file) {
      std::print("unable to open output file\n");
      std::exit(-1);
    }
    rose_defer { std::fclose(state.file); };

    writeFileHeader(state.file, dgc);

    // Setup signal handlers
    {
      struct sigaction action;
      sigemptyset(&action.sa_mask);
      sigaddset(&action.sa_mask, SIGINT);
      sigaddset(&action.sa_mask, SIGTERM);
      action.sa_flags = SA_RESTART;
      action.sa_handler = [](int) { g_stop.store(true); };
      if (sigaction(SIGINT, &action, nullptr) < 0) {
        std::print("sigaction(SIGINT) failed\n");
        std::exit(-1);
      }
      if (sigaction(SIGTERM, &action, nullptr) < 0) {
        std::print("sigaction(SIGTERM) failed\n");
        std::exit(-1);
      }
    }

    std::mt19937_64 per_thread_seed_gen{dgc.root_seed};
    std::vector<std::jthread> threads;
    for (int i = 0; i < thread_count; i++) {
      const u64 thread_seed = per_thread_seed_gen();
      threads.emplace_back(&rose::tool::datagen::threadMain, std::ref(state), dgc, thread_seed);
    }

    for (int i = 0; i < thread_count; i++) {
      threads[i].join();
    }

    std::print("\ndone.\n");
  }

} // namespace rose::tool::datagen

auto main(int argc, char *argv[]) -> int {
  if (argc < 2) {
    std::print("Too few command-line parameters: Missing thread count argument\n");
    std::exit(-1);
  } else if (argc > 2) {
    std::print("Too many command-line parameters\n");
    std::exit(-1);
  }

  const auto thread_count = rose::parseInt(argv[1]);
  if (!thread_count || *thread_count < 1) {
    std::print("Invalid thread count: '{}'\n", argv[1]);
    std::exit(-1);
  }

  rose::tool::datagen::run(*thread_count);
}
