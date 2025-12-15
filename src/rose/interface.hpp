#pragma once

#include "rose/common.hpp"
#include "rose/game.hpp"
#include "rose/util/tokenizer.hpp"

#include <fmt/format.h>
#include <string_view>

namespace rose {

  class Interface {
  private:
    Game game;

    template<typename... Args>
    auto print_protocol_error(std::string_view cmd, fmt::format_string<Args...> fmt, Args&&... args) -> void;
    auto print_unrecognised_token(std::string_view cmd, std::string_view token) -> void;
    auto print_illegal_move(std::string_view move) -> void;
    auto expect_token(std::string_view cmd, Tokenizer& it, std::string_view token) -> bool;

    auto uci_position(Tokenizer& it) -> void;
    auto uci_perft(Tokenizer& it) -> void;

    auto uci_moves(Tokenizer& it) -> void;

  public:
    auto parse_command(std::string_view line) -> void;
  };

}  // namespace rose
