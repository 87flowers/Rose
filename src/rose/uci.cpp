#include "rose/uci.h"

#include <cstdio>
#include <optional>
#include <print>

#include "rose/common.h"
#include "rose/game.h"
#include "rose/perft.h"
#include "rose/util/tokenizer.h"

#define xstr(s) str(s)
#define str(s) #s

namespace rose {

  static auto parseInt(std::string_view str) -> std::optional<i64> {
    bool negate = false;
    i64 result = 0;
    usize i = 0;

    if (str.size() == 0)
      return std::nullopt;

    if (str[0] == '-') {
      if (str.size() == 1)
        return std::nullopt;
      negate = true;
      i = 1;
    }

    for (; i < str.size(); i++) {
      if (str[i] < '0' && str[i] > '9')
        return std::nullopt;
      result = result * 10 + (str[i] - '0');
    }

    return negate ? -result : result;
  }

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

  static auto uciParseMoves(Game &game, Tokenizer &it) -> void {
    while (true) {
      const std::string_view move_str = it.next();
      if (move_str.empty())
        break;
      const auto m = Move::parse(move_str, game.position());
      if (!m)
        return printIllegalMove(move_str);
      game.move(m.value());
    }
  }

  static auto uciParsePosition(Game &game, Tokenizer &it) -> void {
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
      const std::string_view irreversible_clock = it.next();
      const std::string_view ply = it.next();

      const auto pos = Position::parse(board, color, castle, enpassant, irreversible_clock, ply);
      if (!pos)
        return printProtocolError("position", "invalid fen provided: {}", pos.error());
      game.setPosition(pos.value());
    } else {
      return printUnrecognizedToken("position", pos_type);
    }

    if (expectToken("position", it, "moves")) {
      uciParseMoves(game, it);
    }
  }

  static auto uciParseNewGame(Game &game, Tokenizer &it) -> void { game.reset(); }

  static auto uciParseIsReady(Game &game, Tokenizer &it) -> void { std::print("readyok\n"); }

  static auto uciParseUci(Game &game, Tokenizer &it) -> void {
    std::print("id name Rose " ROSE_VERSION "\n"
               "id author 87 (87flowers.com)\n"
               "uciok\n");
  }

  static auto uciParsePerft(Game &game, Tokenizer &it) -> void {
    const std::string_view depth_str = it.rest().empty() ? "1" : it.next();
    const auto depth = parseInt(depth_str);
    if (!depth || *depth < 0)
      return printUnrecognizedToken("perft", depth_str);
    perft::run(game.position(), static_cast<usize>(*depth));
  }

  static auto uciParseUndo(Game &game, Tokenizer &it) -> void {
    const std::string_view count_str = it.rest().empty() ? "1" : it.next();
    const auto count = parseInt(count_str);
    if (!count || *count < 0)
      return printUnrecognizedToken("undo", count_str);
    for (i64 i = 0; i < count; i++)
      game.unmove();
  }

  static auto uciParseDisplay(Game &game, Tokenizer &it) -> void {
    game.position().prettyPrint();

    std::print("fen: {}\n", game.position());
  }

  static auto uciParseCompiler(Game &game, Tokenizer &it) -> void {
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

  auto uciParseCommand(Game &game, std::string_view line) -> void {
    const time::TimePoint start_time = time::Clock::now();

    Tokenizer it{line};
    const std::string_view cmd = it.next();

    if (cmd == "go") {
      std::print("TODO\n");
    } else if (cmd == "position") {
      uciParsePosition(game, it);
    } else if (cmd == "ucinewgame") {
      uciParseNewGame(game, it);
    } else if (cmd == "isready") {
      uciParseIsReady(game, it);
    } else if (cmd == "uci") {
      uciParseUci(game, it);
    } else if (cmd == "perft") {
      uciParsePerft(game, it);
    } else if (cmd == "moves" || cmd == "move") {
      uciParseMoves(game, it);
    } else if (cmd == "undo") {
      uciParseUndo(game, it);
    } else if (cmd == "d") {
      uciParseDisplay(game, it);
    } else if (cmd == "attacks") {
      game.position().attackTable(game.position().activeColor()).dumpRaw();
      game.position().printAttackTable();
    } else if (cmd == "getposition") {
      game.printGameRecord();
    } else if (cmd == "compiler") {
      uciParseCompiler(game, it);
    } else if (cmd == "quit") {
      std::exit(0);
    } else {
      printProtocolError(cmd, "unknown command");
    }
  }

} // namespace rose
