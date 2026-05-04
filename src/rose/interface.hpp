#pragma once

#include "rose/common.hpp"
#include "rose/engine.hpp"
#include "rose/game.hpp"
#include "rose/util/time.hpp"
#include "rose/util/tokenizer.hpp"

#include <fmt/format.h>
#include <string_view>

namespace rose {

  class Interface {
  private:
    Game m_game;
    MoveFormat m_format = MoveFormat::classical;
    Engine m_engine;

    auto set_format(MoveFormat format) -> void;

    template<typename... Args>
    auto print_protocol_error(std::string_view cmd, fmt::format_string<Args...> fmt, Args&&... args) -> void;
    auto print_unrecognised_token(std::string_view cmd, std::string_view token) -> void;
    auto print_illegal_move(std::string_view move) -> void;
    auto expect_token(std::string_view cmd, Tokenizer& it, std::string_view token) -> bool;

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
    auto uci_d(Tokenizer& it) -> void;

  public:
    Interface();
    ~Interface();

    auto parse_command(std::string_view line) -> void;
  };

}  // namespace rose
