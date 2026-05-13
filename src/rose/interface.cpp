#include "rose/interface.hpp"

#include "fmt/base.h"
#include "rose/cmd/bench.hpp"
#include "rose/cmd/perft.hpp"
#include "rose/common.hpp"
#include "rose/engine_output_uci.hpp"
#include "rose/engine_output_xboard.hpp"
#include "rose/position.hpp"
#include "rose/search.hpp"
#include "rose/tt.hpp"
#include "rose/util/string.hpp"
#include "rose/util/time.hpp"
#include "rose/util/tokenizer.hpp"
#include "rose/version.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fmt/format.h>
#include <memory>
#include <string_view>

namespace rose {

  static constexpr usize max_threads = 1024;

  auto Interface::set_format(MoveFormat format) -> void {
    m_format = format;
    switch (m_protocol) {
    case Protocol::uci:
      m_engine.set_output(std::make_shared<EngineOutputUci>(m_format));
      break;
    case Protocol::xboard:
      m_engine.set_output(std::make_shared<EngineOutputXboard<Interface>>(*this, m_format));
      break;
    }
  }

  auto Interface::set_protocol(Protocol protocol) -> void {
    m_protocol = protocol;
    set_format(m_format);
  }

  template<typename... Args>
  auto Interface::print_protocol_error(std::string_view cmd, fmt::format_string<Args...> fmt, Args&&... args) -> void {
    switch (m_protocol) {
    case Protocol::uci:
      fmt::print("info error");
      break;
    case Protocol::xboard:
      fmt::print("Error");
      break;
    }
    fmt::print(" ({}): ", cmd);
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
    std::fflush(stdout);
  }

  auto Interface::print_unrecognised_token(std::string_view cmd, std::string_view token) -> void {
    print_protocol_error(cmd, "unrecognised token `{}`", token);
  }

  auto Interface::print_illegal_move(std::string_view move) -> void {
    switch (m_protocol) {
    case Protocol::uci:
      print_protocol_error("illegal move", "{}", move);
      break;
    case Protocol::xboard:
      fmt::print("Illegal move: {}\n", move);
      break;
    }
  }

  auto Interface::expect_token(std::string_view cmd, Tokenizer& it, std::string_view token) -> bool {
    const std::string_view next = it.next();
    if (next == token)
      return true;
    if (!next.empty()) [[unlikely]]
      print_unrecognised_token(cmd, next);
    return false;
  }

  auto Interface::uci_parse_command(std::string_view line, time::TimePoint start_time) -> void {
    Tokenizer it {line};
    const std::string_view cmd = it.next();

    if (cmd == "position") {
      uci_position(it);
    } else if (cmd == "go") {
      uci_go(it, start_time);
    } else if (cmd == "ucinewgame") {
      uci_ucinewgame(it);
    } else if (cmd == "uci") {
      uci_uci(it);
    } else if (cmd == "isready") {
      uci_isready(it);
    } else if (cmd == "setoption") {
      uci_setoption(it);
    } else if (cmd == "stop") {
      uci_stop(it);
    } else if (cmd == "perft") {
      uci_perft(it);
    } else if (cmd == "bench") {
      uci_bench(it);
    } else if (cmd == "moves") {
      uci_moves(it);
    } else if (cmd == "d") {
      cmd_d(it);
    } else if (cmd == "getposition") {
      m_game.print_game_record();
    } else if (cmd == "dumpposition") {
      m_game.position().dump();
    } else if (cmd == "hashstack") {
      m_game.print_hash_stack();
    } else if (cmd == "wait") {
      m_engine.wait();
    } else if (cmd == "quit") {
      std::exit(0);
    } else if (cmd == "xboard") {
      set_protocol(Protocol::xboard);
      fmt::print("\n");
      std::fflush(stdout);
    } else {
      print_protocol_error(cmd, "unknown command");
    }
  }

  auto Interface::uci_position(Tokenizer& it) -> void {
    const std::string_view pos_type = it.next();
    if (pos_type.empty()) [[unlikely]]
      return print_protocol_error("position", "no position provided");

    if (pos_type == "startpos") {
      m_game.set_position(Position::startpos());
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
      m_game.set_position(pos.value());
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

    m_engine.run_search(start_time, limits, m_game);
  }

  auto Interface::uci_ucinewgame(Tokenizer&) -> void {
    m_engine.reset();
  }

  auto Interface::uci_uci(Tokenizer&) -> void {
    fmt::print("id name Rose {}\n", rose::version::to_string());
    fmt::print("id author 87 (87flowers.com)\n");
    fmt::print("option name Hash type spin default {} min 1 max {}\n", tt::default_hash_size_mb, tt::maximum_hash_size_mb);
    fmt::print("option name Threads type spin default 1 min 1 max {}\n", max_threads);
    fmt::print("option name UCI_Chess960 type check default false\n");
    fmt::print("uciok\n");
  }

  auto Interface::uci_isready(Tokenizer&) -> void {
    fmt::print("readyok\n");
  }

  auto Interface::uci_setoption(Tokenizer& it) -> void {
    if (!expect_token("setoption", it, "name"))
      return;
    const std::string_view name = it.next();

    if (!expect_token("setoption", it, "value"))
      return;
    const std::string_view value = it.next();

    if (name == "Hash") {
      const auto hash = parse_int(value);
      if (!hash || *hash <= 0)
        return print_unrecognised_token("setoption", value);
      m_engine.set_hash_size(*hash);
    } else if (name == "Threads") {
      const auto count = parse_int(value);
      if (!count || *count <= 0)
        return print_unrecognised_token("setoption", value);
      m_engine.set_thread_count(*count);
    } else if (name == "UCI_Chess960") {
      if (value == "true") {
        set_format(MoveFormat::frc);
      } else if (value == "false") {
        set_format(MoveFormat::classical);
      } else {
        return print_unrecognised_token("setoption", value);
      }
    } else {
      return print_unrecognised_token("setoption", name);
    }
  }

  auto Interface::uci_stop(Tokenizer& it) -> void {
    m_engine.stop();
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

    perft::run(m_game.position(), static_cast<usize>(*depth), bulk);
  }

  auto Interface::uci_bench(Tokenizer&) -> void {
    bench::run();
  }

  auto Interface::uci_moves(Tokenizer& it) -> void {
    while (true) {
      const std::string_view move_str = it.next();
      if (move_str.empty())
        break;
      const auto m = Move::parse(move_str, m_format, m_game.position());
      if (!m) [[unlikely]]
        return print_illegal_move(move_str);
      m_game.move(m.value());
    }
  }

  auto Interface::xboard_parse_command(std::string_view line, time::TimePoint start_time) -> void {
    Tokenizer it {line};
    const std::string_view cmd = it.next();

    if (cmd == "protover") {
      xboard_protover(it);
    } else if (cmd == "accepted" || cmd == "rejected") {
      // ignore
    } else if (cmd == "new") {
      xboard_new(it);
    } else if (cmd == "variant") {
      xboard_variant(it);
    } else if (cmd == "quit") {
      std::exit(0);
    } else if (cmd == "random") {
      // ignore
    } else if (cmd == "force") {
      xboard_force(it);
    } else if (cmd == "go") {
      xboard_go(start_time);
    } else if (cmd == "level") {
      xboard_level(it);
    } else if (cmd == "st") {
      xboard_st(it);
    } else if (cmd == "sd") {
      xboard_sd(it);
    } else if (cmd == "time") {
      xboard_time(it);
    } else if (cmd == "otim") {
      xboard_otim(it);
    } else if (cmd == "usermove") {
      xboard_usermove(it.next(), start_time);
    } else if (cmd == "?") {
      m_engine.stop();
    } else if (cmd == "ping") {
      xboard_ping(it);
    } else if (cmd == "result") {
      m_engine.stop();
    } else if (cmd == "setboard") {
      xboard_setboard(it);
    } else if (cmd == "undo") {
      xboard_undo(it);
    } else if (cmd == "memory") {
      xboard_memory(it);
    } else if (cmd == "cores") {
      xboard_cores(it);
    } else if (cmd == "d") {
      cmd_d(it);
    } else if (cmd == "getposition") {
      m_game.print_game_record();
    } else if (cmd == "dumpposition") {
      m_game.position().dump();
    } else if (cmd == "hashstack") {
      m_game.print_hash_stack();
    } else if (cmd == "wait") {
      m_engine.wait();
    } else if (cmd == "uci") {
      set_protocol(Protocol::uci);
      uci_uci(it);
    } else if (cmd == "O-O" || cmd == "O-O-O" || std::find_if(cmd.begin(), cmd.end(), [](char ch) {
                                                   return ch >= '0' && ch <= '9';
                                                 }) != cmd.end()) {
      xboard_usermove(cmd, start_time);
    } else {
      print_protocol_error(cmd, "unknown command");
    }
  }

  auto Interface::xboard_engine_moved(Move m) -> void {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    m_game.move(m);
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }

  auto Interface::xboard_protover(Tokenizer&) -> void {
    fmt::print("feature myname=\"Rose {}\"\n", rose::version::to_string());
    fmt::print("feature variants=\"normal,fischerandom\"\n");
    fmt::print("feature sigint=0 sigterm=0 ping=1 debug=1\n");
    fmt::print("feature colors=0 setboard=1 san=0\n");
    fmt::print("feature draw=0 analyze=0\n");
    fmt::print("feature memory=1 smp=1\n");

    // UCI-isms
    fmt::print("feature option=\"Hash -spin {} 1 {}\"\n", tt::default_hash_size_mb, tt::maximum_hash_size_mb);
    fmt::print("feature option=\"Threads -spin 1 1 {}\"\n", max_threads);

    fmt::print("feature done=1\n");
  }

  auto Interface::xboard_new(Tokenizer&) -> void {
    m_engine.reset();
    m_game.reset();
    set_format(MoveFormat::classical);
    m_xboard.force = false;
    m_xboard.tc_sd = std::nullopt;
    if (m_xboard.tc_base) {
      m_xboard.clocks[0] = *m_xboard.tc_base;
      m_xboard.clocks[1] = *m_xboard.tc_base;
    }
  }

  auto Interface::xboard_variant(Tokenizer& it) -> void {
    const std::string_view variant = it.next();

    if (variant == "normal") {
      set_format(MoveFormat::classical);
    } else if (variant == "fischerandom") {
      set_format(MoveFormat::frc);
    } else {
      print_unrecognised_token("variant", variant);
    }
  }

  auto Interface::xboard_force(Tokenizer&) -> void {
    m_xboard.force = true;
  }

  auto Interface::xboard_go(time::TimePoint start_time) -> void {
    m_xboard.force = false;

    SearchLimit limits;

    limits.has_time = true;
    if (m_xboard.tc_st) {
      limits.movetime = time::cast<time::Milliseconds>(*m_xboard.tc_st).count();
    } else {
      limits.wtime = time::cast<time::Milliseconds>(m_xboard.clocks[0]).count();
      limits.btime = time::cast<time::Milliseconds>(m_xboard.clocks[1]).count();
      if (m_xboard.tc_increment) {
        limits.winc = time::cast<time::Milliseconds>(*m_xboard.tc_increment).count();
        limits.binc = time::cast<time::Milliseconds>(*m_xboard.tc_increment).count();
      }
    }

    if (m_xboard.tc_sd) {
      limits.has_other = true;
      limits.depth = m_xboard.tc_sd;
    }

    m_engine.run_search(start_time, limits, m_game);
  }

  auto Interface::xboard_level(Tokenizer& it) -> void {
    const std::string_view mpt_str = it.next();
    const std::string_view base_str = it.next();
    const std::string_view inc_str = it.next();

    if (mpt_str != "0") {
      fmt::print("# Warning: Rose currently pretends repeating time control periods don't exist\n");
    }

    f64 base;
    if (base_str.contains(':')) {
      const auto split = string_split(std::string {base_str}, ':');
      const auto mins = parse_f64(split[0]);
      const auto secs = parse_f64(split[1]);
      if (split.size() > 2 || !mins || !secs)
        return print_protocol_error("level", "invalid base value: {}", base_str);
      base = *mins * 60 + *secs;
    } else {
      const auto mins = parse_f64(base_str);
      if (!mins)
        return print_protocol_error("level", "invalid base value: {}", base_str);
      base = *mins * 60;
    }

    const auto inc = parse_f64(inc_str);
    if (!inc)
      return print_protocol_error("level", "invalid inc value: {}", inc_str);

    m_xboard.tc_base = time::cast<time::Duration>(time::FloatSeconds {base});
    m_xboard.tc_increment = time::cast<time::Duration>(time::FloatSeconds {*inc});
    m_xboard.tc_st = std::nullopt;
  }

  auto Interface::xboard_st(Tokenizer& it) -> void {
    const std::string_view st_str = it.next();
    const auto st = parse_f64(st_str);
    if (!st)
      return print_protocol_error("st", "invalid st value: {}", st_str);

    m_xboard.tc_base = std::nullopt;
    m_xboard.tc_increment = std::nullopt;
    m_xboard.tc_st = time::cast<time::Duration>(time::FloatSeconds {*st});
  }

  auto Interface::xboard_sd(Tokenizer& it) -> void {
    const std::string_view sd_str = it.next();
    const auto sd = parse_int(sd_str);
    if (!sd || *sd <= 0)
      return print_protocol_error("sd", "invalid sd value: {}", sd_str);

    m_xboard.tc_sd = sd;
  }

  auto Interface::xboard_time(Tokenizer& it) -> void {
    const std::string_view value = it.next();
    const auto time = parse_f64(value);
    if (!time)
      return print_protocol_error("time", "invalid value: {}", value);

    m_xboard.clocks[m_game.position().stm().invert().to_index()] = time::cast<time::Duration>(time::FloatSeconds {*time});
  }

  auto Interface::xboard_otim(Tokenizer& it) -> void {
    const std::string_view value = it.next();
    const auto time = parse_f64(value);
    if (!time)
      return print_protocol_error("otim", "invalid value: {}", value);

    m_xboard.clocks[m_game.position().stm().to_index()] = time::cast<time::Duration>(time::FloatSeconds {*time});
  }

  auto Interface::xboard_usermove(std::string_view move_str, time::TimePoint start_time) -> void {
    const Position& pos = m_game.position();
    const Move m = [&] {
      const Color stm = pos.stm();
      if (move_str == "O-O") {
        return Move::make(pos.king_sq(stm), pos.rook_info().hside(stm), MoveFlags::castle_hside);
      }
      if (move_str == "O-O-O") {
        return Move::make(pos.king_sq(stm), pos.rook_info().aside(stm), MoveFlags::castle_aside);
      }
      return Move::parse(move_str, m_format, pos).value_or(Move::none());
    }();

    if (!pos.is_legal(m))
      return print_illegal_move(move_str);

    m_game.move(m);

    if (!m_xboard.force)
      xboard_go(start_time);
  }

  auto Interface::xboard_ping(Tokenizer& it) -> void {
    fmt::print("pong {}\n", it.rest());
  }

  auto Interface::xboard_setboard(Tokenizer& it) -> void {
    const auto pos = Position::parse(it.rest());
    if (!pos)
      return print_protocol_error("setboard", "invalid fen provided: {}", pos.error());

    m_engine.wait();
    m_game.set_position(pos.value());
  }

  auto Interface::xboard_undo(Tokenizer&) -> void {
    m_engine.wait();
    m_game.unmove();
  }

  auto Interface::xboard_memory(Tokenizer& it) -> void {
    const std::string_view value = it.next();
    const auto hash = parse_int(value);
    if (!hash || *hash <= 0)
      return print_unrecognised_token("memory", value);

    m_engine.wait();
    m_engine.set_hash_size(*hash);
  }

  auto Interface::xboard_cores(Tokenizer& it) -> void {
    const std::string_view value = it.next();
    const auto threads = parse_int(value);
    if (!threads || *threads <= 0)
      return print_unrecognised_token("cores", value);

    m_engine.wait();
    m_engine.set_thread_count(*threads);
  }

  auto Interface::cmd_d(Tokenizer&) -> void {
    const Position& pos = m_game.position();

    fmt::print("   +------------------------+\n");
    for (i8 rank = 7; rank >= 0; rank--) {
      fmt::print(" {} |", static_cast<char>('1' + rank));
      for (i8 file = 0; file < 8; file++) {
        const Square sq = Square::from_file_and_rank(file, rank);
        const Place place = pos.place_at(sq);
        fmt::print(" {} ", place);
      }
      fmt::print("|\n");
    }
    fmt::print("   +------------------------+\n");
    fmt::print("     a  b  c  d  e  f  g  h  \n");
    fmt::print("\n");
    fmt::print("fen: {}\n", pos.to_string(MoveFormat::frc));
    fmt::print("hash: {:016x}\n", m_game.hash());
  }

  Interface::Interface() {
    set_format(m_format);
  }

  Interface::~Interface() = default;

  auto Interface::parse_command(std::string_view line) -> void {
    const time::TimePoint start_time = time::Clock::now();

    switch (m_protocol) {
    case Protocol::uci:
      return uci_parse_command(line, start_time);
    case Protocol::xboard:
      return xboard_parse_command(line, start_time);
    }
  }

}  // namespace rose
