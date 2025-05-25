#include "rose/uci.h"

#include <cstdio>
#include <optional>
#include <print>

#include "rose/cmd/bench.h"
#include "rose/cmd/check_perft.h"
#include "rose/cmd/perft.h"
#include "rose/common.h"
#include "rose/config.h"
#include "rose/engine.h"
#include "rose/eval/hce.h"
#include "rose/game.h"
#include "rose/hash.h"
#include "rose/tt.h"
#include "rose/util/defer.h"
#include "rose/util/string.h"
#include "rose/util/tokenizer.h"

#define xstr(s) str(s)
#define str(s) #s

namespace rose {

  template <typename... Args> static auto printProtocolError(std::string_view cmd, std::format_string<Args...> fmt, Args &&...args) -> void {
    std::print("error ({}): ", cmd);
    std::print(fmt, std::forward<Args>(args)...);
    std::print("\n");
    std::fflush(stdout);
  }

  static auto printUnrecognizedToken(std::string_view cmd, std::string_view token) -> void {
    printProtocolError(cmd, "unrecognised token `{}`", token);
  }

  static auto printIllegalMove(std::string_view move) -> void { printProtocolError("illegal move", "{}", move); }

  static auto expectToken(std::string_view cmd, Tokenizer &it, std::string_view token) -> bool {
    const std::string_view next = it.next();
    if (next == token)
      return true;
    if (!next.empty())
      printUnrecognizedToken(cmd, next);
    return false;
  }

  static auto uciParseGo(Engine &engine, Game &game, Tokenizer &it, time::TimePoint start_time) -> void {
    SearchLimit limits;
    for (std::string_view part = it.next(); !part.empty(); part = it.next()) {
#define DO_PART(section, x)                                                                                                                          \
  limits.has_##section = true;                                                                                                                       \
  const std::string_view value_str = it.next();                                                                                                      \
  const auto value = parseInt(value_str);                                                                                                            \
  if (!value) [[unlikely]]                                                                                                                           \
    return printUnrecognizedToken("go", value_str);                                                                                                  \
  limits.x = std::max<int>(0, *value);
      if (part == "wtime") {
        DO_PART(time, wtime);
      } else if (part == "btime") {
        DO_PART(time, btime);
      } else if (part == "winc") {
        DO_PART(time, winc);
      } else if (part == "binc") {
        DO_PART(time, binc);
      } else if (part == "movestogo") {
        DO_PART(time, movestogo);
      } else if (part == "movetime") {
        DO_PART(time, movetime);
      } else if (part == "nodes") {
        DO_PART(other, hard_nodes);
        if (!limits.soft_nodes)
          limits.soft_nodes = limits.hard_nodes;
      } else if (part == "softnodes") {
        DO_PART(other, soft_nodes);
      } else if (part == "depth") {
        DO_PART(other, depth);
      } else [[unlikely]] if (part == "perft") {
        return printProtocolError("go", "perft is a standalone command in Rose, and is not a subcommand of go");
      } else {
        return printUnrecognizedToken("go", part);
      }
#undef DO_PART
    }
    engine.runSearch(start_time, limits);
  }

  static auto uciParseMoves(Engine &engine, Game &game, Tokenizer &it) -> void {
    rose_defer { engine.setGame(game); };
    while (true) {
      const std::string_view move_str = it.next();
      if (move_str.empty())
        break;
      const auto m = Move::parse(move_str, game.position());
      if (!m) [[unlikely]]
        return printIllegalMove(move_str);
      game.move(m.value());
    }
  }

  static auto uciParsePosition(Engine &engine, Game &game, Tokenizer &it) -> void {
    const std::string_view pos_type = it.next();
    if (pos_type.empty())
      return printProtocolError("position", "no position provided");

    if (pos_type == "startpos") {
      game.setPositionStartpos();
    } else if (pos_type == "fen") {
      const std::string_view board = it.next();
      const std::string_view color = it.next();
      const std::string_view castle = it.next();
      const std::string_view enpassant = it.next();
      const std::string_view clock_50mr = it.next();
      const std::string_view ply = it.next();

      const auto pos = Position::parse(board, color, castle, enpassant, clock_50mr, ply);
      if (!pos)
        return printProtocolError("position", "invalid fen provided: {}", pos.error());
      game.setPosition(pos.value());
    } else {
      return printUnrecognizedToken("position", pos_type);
    }

    if (expectToken("position", it, "moves")) {
      uciParseMoves(engine, game, it);
    } else {
      engine.setGame(game);
    }
  }

  static auto uciParseNewGame(Engine &engine, Game &game, Tokenizer &it) -> void {
    game.reset();
    engine.reset();
    engine.setGame(game);
  }

  static auto uciParseIsReady(Engine &engine, Game &game, Tokenizer &it) -> void {
    engine.isReady();
    std::print("readyok\n");
  }

  static auto uciParseStop(Engine &engine, Game &game, Tokenizer &it) -> void { engine.stop(); }

  static auto uciParseUci(Engine &engine, Game &game, Tokenizer &it) -> void {
    static constexpr auto max_threads = std::min<std::ptrdiff_t>(std::barrier<>::max(), std::numeric_limits<int>::max());

    std::print("id name Rose " ROSE_VERSION "\n"
               "id author 87 (87flowers.com)\n"
               "option name Hash type spin default {} min 1 max 1048576\n"
               "option name Threads type spin default 1 min 1 max {}\n"
               "option name UCI_Chess960 type check default false\n"
               "uciok\n",
               tt::default_hash_size_mb, max_threads);
  }

  static auto uciParseSetOption(Engine &engine, Game &game, Tokenizer &it) -> void {
    if (!expectToken("setoption", it, "name"))
      return;
    const std::string_view name = it.next();
    if (!expectToken("setoption", it, "value"))
      return;
    const std::string_view value = it.next();
    if (name == "Hash") {
      const auto hash = parseInt(value);
      if (!hash || *hash <= 0)
        return printUnrecognizedToken("setoption", value);
      engine.setHashSize(*hash);
    } else if (name == "Threads") {
      const auto count = parseInt(value);
      if (!count || *count <= 0)
        return printUnrecognizedToken("setoption", value);
      engine.setThreadCount(*count);
    } else if (name == "UCI_Chess960") {
      if (value == "true") {
        config::frc = true;
      } else if (value == "false") {
        config::frc = false;
      } else {
        return printUnrecognizedToken("setoption", value);
      }
    } else {
      return printUnrecognizedToken("setoption", name);
    }
  }

  static auto uciParsePerft(Engine &engine, Game &game, Tokenizer &it) -> void {
    const std::string_view depth_str = it.rest().empty() ? "1" : it.next();
    const auto depth = parseInt(depth_str);
    if (!depth || *depth < 0)
      return printUnrecognizedToken("perft", depth_str);
    perft::run(game.position(), static_cast<usize>(*depth));
  }

  static auto uciParseUndo(Engine &engine, Game &game, Tokenizer &it) -> void {
    const std::string_view count_str = it.rest().empty() ? "1" : it.next();
    const auto count = parseInt(count_str);
    if (!count || *count < 0)
      return printUnrecognizedToken("undo", count_str);
    for (i64 i = 0; i < count; i++)
      game.unmove();
    engine.setGame(game);
  }

  static auto uciParseDisplay(Engine &engine, Game &game, Tokenizer &it) -> void {
    game.position().prettyPrint();

    std::print("fen: {}\n", game.position());
  }

  static auto uciParseCompiler(Engine &engine, Game &game, Tokenizer &it) -> void {
    // clang-format off
    std::print("compiler build-datetime " __DATE__ " " __TIME__ "\n"
#if defined(__VERSION__)
               "compiler version " __VERSION__ "\n"
#endif
#if defined(__clang__)
               "compiler family clang++ version " xstr(__clang_major__) "." xstr(__clang_minor__) "." xstr(__clang_patchlevel__) "\n"
#elif defined(__GNUC__)
               "compiler family g++ version " xstr(__GNUC__) "." xstr(__GNUC_MINOR__) "." xstr(__GNUC_PATCHLEVEL__) "\n"
#elif defined(_MSC_VER)
               "compiler family msvc version " xstr(_MSC_FULL_VER) " " xstr(_MSC_BUILD) "\n"
#else
               "compiler family unknown\n"
#endif
#if ROSE_NO_ASSERTS
               "compiler assertions disabled\n"
#else
               "compiler assertions enabled\n"
#endif
               "compilerok\n");
    // clang-format on
  }

  static auto uciParseHashes(Engine &engine, Game &game, Tokenizer &it) -> void {
    for (int ptype = 0; ptype < 16; ptype++) {
      std::print("piece_table[{}]:", ptype);
      for (u64 h : hash::piece_table[ptype])
        std::print(" {:016x}", h);
      std::print("\n");
    }

    {
      std::print("enpassant_table:");
      for (u64 h : hash::enpassant_table)
        std::print(" {:016x}", h);
      std::print("\n");
    }

    for (int color = 0; color < 2; color++) {
      std::print("castle_table[{}]:", color);
      for (u64 h : hash::castle_table[color])
        std::print(" {:016x}", h);
      std::print("\n");
    }

    {
      std::print("move: {:016x}\n", hash::move);
    }
  }

  auto uciParseCommand(Engine &engine, Game &game, std::string_view line) -> void {
    const time::TimePoint start_time = time::Clock::now();

    Tokenizer it{line};
    const std::string_view cmd = it.next();

    if (cmd == "go") {
      uciParseGo(engine, game, it, start_time);
    } else if (cmd == "position") {
      uciParsePosition(engine, game, it);
    } else if (cmd == "ucinewgame") {
      uciParseNewGame(engine, game, it);
    } else if (cmd == "isready") {
      uciParseIsReady(engine, game, it);
    } else if (cmd == "stop") {
      uciParseStop(engine, game, it);
    } else if (cmd == "uci") {
      uciParseUci(engine, game, it);
    } else if (cmd == "setoption") {
      uciParseSetOption(engine, game, it);
    } else if (cmd == "perft") {
      uciParsePerft(engine, game, it);
    } else if (cmd == "check_perft") {
      check_perft::run(it.rest());
    } else if (cmd == "bench") {
      bench::run(engine, game);
    } else if (cmd == "moves" || cmd == "move") {
      uciParseMoves(engine, game, it);
    } else if (cmd == "undo") {
      uciParseUndo(engine, game, it);
    } else if (cmd == "d") {
      uciParseDisplay(engine, game, it);
    } else if (cmd == "attacks") {
      game.position().attackTable(game.position().activeColor()).dumpRaw();
      game.position().printAttackTable();
    } else if (cmd == "getposition") {
      game.printGameRecord();
    } else if (cmd == "eval") {
      std::print("score cp {}\n", eval::hce(game.position()));
    } else if (cmd == "compiler") {
      uciParseCompiler(engine, game, it);
    } else if (cmd == "hashes") {
      uciParseHashes(engine, game, it);
    } else if (cmd == "wait") {
      engine.isReady();
    } else if (cmd == "quit") {
      std::exit(0);
    } else {
      printProtocolError(cmd, "unknown command");
    }
  }

} // namespace rose
