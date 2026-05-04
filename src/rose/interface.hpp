#pragma once

#include "rose/common.hpp"
#include "rose/engine.hpp"
#include "rose/engine_output_xboard.hpp"
#include "rose/game.hpp"
#include "rose/util/time.hpp"
#include "rose/util/tokenizer.hpp"
#include "rose/xboard.hpp"

#include <array>
#include <fmt/format.h>
#include <string_view>

namespace rose {

  class Interface {
  private:
    enum class Protocol {
      uci,
      xboard,
    };

    Game m_game;
    Engine m_engine;
    MoveFormat m_format = MoveFormat::classical;
    Protocol m_protocol = Protocol::uci;

    XBoardState m_xboard;

    auto set_format(MoveFormat format) -> void;
    auto set_protocol(Protocol protocol) -> void;

    template<typename... Args>
    auto print_protocol_error(std::string_view cmd, fmt::format_string<Args...> fmt, Args&&... args) -> void;
    auto print_unrecognised_token(std::string_view cmd, std::string_view token) -> void;
    auto print_illegal_move(std::string_view move) -> void;
    auto expect_token(std::string_view cmd, Tokenizer& it, std::string_view token) -> bool;

    auto uci_parse_command(std::string_view line, time::TimePoint start_time) -> void;

    auto uci_position(Tokenizer& it) -> void;
    auto uci_go(Tokenizer& it, time::TimePoint start_time) -> void;
    auto uci_ucinewgame(Tokenizer& it) -> void;
    auto uci_uci(Tokenizer& it) -> void;
    auto uci_isready(Tokenizer& it) -> void;
    auto uci_setoption(Tokenizer& it) -> void;
    auto uci_stop(Tokenizer& it) -> void;
    auto uci_perft(Tokenizer& it) -> void;
    auto uci_bench(Tokenizer& it) -> void;
    auto uci_moves(Tokenizer& it) -> void;

    auto xboard_parse_command(std::string_view line, time::TimePoint start_time) -> void;

    friend struct EngineOutputXboard<Interface>;
    auto xboard_engine_moved(Move m) -> void;

    auto xboard_protover(Tokenizer& it) -> void;
    auto xboard_variant(Tokenizer& it) -> void;
    auto xboard_new(Tokenizer& it) -> void;
    auto xboard_force(Tokenizer& it) -> void;
    auto xboard_go(time::TimePoint start_time) -> void;
    auto xboard_level(Tokenizer& it) -> void;
    auto xboard_st(Tokenizer& it) -> void;
    auto xboard_sd(Tokenizer& it) -> void;
    auto xboard_time(Tokenizer& it) -> void;
    auto xboard_otim(Tokenizer& it) -> void;
    auto xboard_usermove(std::string_view move, time::TimePoint start_time) -> void;
    auto xboard_ping(Tokenizer& it) -> void;
    auto xboard_setboard(Tokenizer& it) -> void;
    auto xboard_undo(Tokenizer& it) -> void;
    auto xboard_memory(Tokenizer& it) -> void;
    auto xboard_cores(Tokenizer& it) -> void;

    auto cmd_d(Tokenizer& it) -> void;

  public:
    Interface();
    ~Interface();

    auto parse_command(std::string_view line) -> void;
  };

}  // namespace rose
