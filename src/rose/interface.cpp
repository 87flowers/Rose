#include "rose/interface.hpp"

#include "rose/cmd/perft.hpp"
#include "rose/common.hpp"
#include "rose/engine_output_uci.hpp"
#include "rose/position.hpp"
#include "rose/search.hpp"
#include "rose/util/string.hpp"
#include "rose/util/time.hpp"
#include "rose/util/tokenizer.hpp"

#include <cstdio>
#include <fmt/format.h>
#include <memory>
#include <string_view>

namespace rose {

  template<typename... Args>
  auto Interface::print_protocol_error(std::string_view cmd, fmt::format_string<Args...> fmt, Args&&... args) -> void {
    fmt::print("info error ({}): ", cmd);
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
    std::fflush(stdout);
  }

  auto Interface::print_unrecognised_token(std::string_view cmd, std::string_view token) -> void {
    print_protocol_error(cmd, "unrecognised token `{}`", token);
  }

  auto Interface::print_illegal_move(std::string_view move) -> void {
    print_protocol_error("illegal move", "{}", move);
  }

  auto Interface::expect_token(std::string_view cmd, Tokenizer& it, std::string_view token) -> bool {
    const std::string_view next = it.next();
    if (next == token)
      return true;
    if (!next.empty()) [[unlikely]]
      print_unrecognised_token(cmd, next);
    return false;
  }

  auto Interface::uci_position(Tokenizer& it) -> void {
    const std::string_view pos_type = it.next();
    if (pos_type.empty()) [[unlikely]]
      return print_protocol_error("position", "no position provided");

    if (pos_type == "startpos") {
      game.set_position(Position::startpos());
    } else if (pos_type == "fen") {
      const std::string_view board = it.next();
      const std::string_view color = it.next();
      const std::string_view castle = it.next();
      const std::string_view enpassant = it.next();
      const std::string_view clock_50mr = it.next();
      const std::string_view ply = it.next();

      const auto pos = Position::parse(board, color, castle, enpassant, clock_50mr, ply);
      if (!pos) [[unlikely]]
        return print_protocol_error("position", "invalid fen provided: {}", pos.error());
      game.set_position(pos.value());
    } else [[unlikely]] {
      return print_unrecognised_token("position", pos_type);
    }

    if (expect_token("position", it, "moves")) {
      uci_moves(it);
    }
  }

  auto Interface::uci_go(Tokenizer& it, time::TimePoint start_time) -> void {
    SearchLimit limits;

#define DO_PART(section, member)                                                                                                                     \
  limits.has_##section = true;                                                                                                                       \
  const std::string_view value_str = it.next();                                                                                                      \
  const auto value = parse_int(value_str);                                                                                                           \
  if (!value)                                                                                                                                        \
    return print_unrecognised_token("go", value_str);                                                                                                \
  limits.member = std::max<int>(0, *value);

    for (std::string_view part = it.next(); !part.empty(); part = it.next()) {
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
      } else if (part == "perft") {
        return print_protocol_error("go", "perft is a standalone command in Rose, and is not a subcommand of go");
      } else {
        return print_unrecognised_token("go", part);
      }
    }

#undef DO_PART

    engine.run_search(start_time, limits, game);
  }

  auto Interface::uci_perft(Tokenizer& it) -> void {
    const std::string_view depth_str = it.rest().empty() ? "1" : it.next();
    const auto depth = parse_int(depth_str);
    if (!depth || *depth < 0)
      return print_unrecognised_token("perft", depth_str);

    const std::string_view bulk_str = it.rest().empty() ? "bulk" : it.next();
    bool bulk;
    if (bulk_str == "bulk")
      bulk = true;
    else if (bulk_str == "nonbulk" || bulk_str == "nobulk")
      bulk = false;
    else
      return print_unrecognised_token("perft", bulk_str);

    perft::run(game.position(), static_cast<usize>(*depth), bulk);
  }

  auto Interface::uci_moves(Tokenizer& it) -> void {
    while (true) {
      const std::string_view move_str = it.next();
      if (move_str.empty())
        break;
      const auto m = Move::parse(move_str, format, game.position());
      if (!m) [[unlikely]]
        return print_illegal_move(move_str);
      game.move(m.value());
    }
  }

  Interface::Interface() {
    engine.set_output(std::make_shared<EngineOutputUci>(format));
  }

  Interface::~Interface() = default;

  auto Interface::parse_command(std::string_view line) -> void {
    const time::TimePoint start_time = time::Clock::now();

    Tokenizer it {line};
    const std::string_view cmd = it.next();

    if (cmd == "position") {
      uci_position(it);
    } else if (cmd == "go") {
      uci_go(it, start_time);
    } else if (cmd == "perft") {
      uci_perft(it);
    } else if (cmd == "getposition") {
      game.print_game_record();
    } else if (cmd == "dumpposition") {
      game.position().dump();
    } else if (cmd == "hashstack") {
      game.print_hash_stack();
    } else if (cmd == "quit") {
      std::exit(0);
    } else {
      print_protocol_error(cmd, "unknown command");
    }
  }

}  // namespace rose
