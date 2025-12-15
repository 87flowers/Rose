#pragma once

#include "rose/common.hpp"
#include "rose/move.hpp"
#include "rose/position.hpp"
#include "rose/util/assert.hpp"

#include <vector>

namespace rose {

  struct Game {
  private:
    std::vector<Position> m_position_stack;
    std::vector<Move> m_move_stack;
    // std::vector<u64> m_hash_stack;

  public:
    Game() {
      reset();
    }

    auto reset() -> void {
      set_position(Position::startpos());
    }

    auto position() const -> const Position& {
      return m_position_stack.back();
    }

    // auto hash() const -> u64 {
    //   return m_hash_stack.back();
    // }

    auto move_stack() const -> std::vector<Move> {
      return m_move_stack;
    }

    // auto hashStack() const -> std::vector<u64> {
    //   return m_hash_stack;
    // }

    auto set_position(const Position& new_pos) -> void {
      m_position_stack.clear();
      m_move_stack.clear();
      // m_hash_stack.clear();

      m_position_stack.push_back(new_pos);
      // m_hash_stack.push_back(new_pos.hash());
    }

    auto move(Move m) -> void {
      m_position_stack.emplace_back(m_position_stack.back().move(m));
      m_move_stack.push_back(m);
      // m_hash_stack.push_back(m_position_stack.back().hash());
    }

    auto unmove() -> void {
      m_position_stack.pop_back();
      m_move_stack.pop_back();
      // m_hash_stack.pop_back();
    }

    auto print_game_record() const -> void;
    // auto print_hash_stack() const -> void;
  };

}  // namespace rose
