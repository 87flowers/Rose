#include "rose/interface.hpp"

#include "rose/cmd/perft.hpp"
#include "rose/common.hpp"
#include "rose/position.hpp"
#include "rose/util/string.hpp"
#include "rose/util/time.hpp"
#include "rose/util/tokenizer.hpp"

#include <cstdio>
#include <fmt/format.h>
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

  auto Interface::uci_perft(Tokenizer& it) -> void {
    const std::string_view depth_str = it.rest().empty() ? "1" : it.next();
    const auto depth = parse_int(depth_str);
    if (!depth || *depth < 0)
      return print_unrecognised_token("perft", depth_str);
    perft::run(game.position(), static_cast<usize>(*depth), false);
  }

  auto Interface::uci_moves(Tokenizer& it) -> void {
    while (true) {
      const std::string_view move_str = it.next();
      if (move_str.empty())
        break;
      const auto m = Move::parse(move_str, game.position());
      if (!m) [[unlikely]]
        return print_illegal_move(move_str);
      game.move(m.value());
    }
  }

  auto Interface::parse_command(std::string_view line) -> void {
    const time::TimePoint start_time = time::Clock::now();

    Tokenizer it {line};
    const std::string_view cmd = it.next();

    if (cmd == "position") {
      uci_position(it);
    } else if (cmd == "perft") {
      uci_perft(it);
    } else if (cmd == "getposition") {
      game.print_game_record();
    } else if (cmd == "dumpposition") {
      game.position().dump();
    } else if (cmd == "quit") {
      std::exit(0);
    } else {
      print_protocol_error(cmd, "unknown command");
    }
  }

}  // namespace rose
