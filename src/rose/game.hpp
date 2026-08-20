#pragma once

#include "rose/is_draw.hpp"
#include "rose/move.hpp"
#include "rose/position.hpp"

#include <vector>

namespace rose {

  struct Game {
  private:
    std::vector<Move> m_move_stack;
    std::vector<Hashes> m_hash_stack;
    std::vector<Position> m_position_stack;

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

    auto initial_position() const -> const Position& {
      return m_position_stack.front();
    }

    auto hash() const -> Hash {
      return m_hash_stack.back().full();
    }

    auto move_stack() const -> std::vector<Move> {
      return m_move_stack;
    }

    auto hash_stack() const -> std::vector<Hashes> {
      return m_hash_stack;
    }

    auto is_draw_slow() const -> bool {
      const Position& pos = position();
      return draw::is_stalemate(pos) || draw::is_fifty_move_draw(pos, 0) == 0 || draw::is_repetition(pos, hash_stack(), hash_stack().size());
    }

    auto set_position(const Position& new_pos) -> void {
      m_move_stack.clear();
      m_hash_stack.clear();
      m_position_stack.clear();

      m_hash_stack.push_back(new_pos.calc_hashes_slow());
      m_position_stack.push_back(new_pos);
    }

    auto move(Move m) -> void {
      m_move_stack.push_back(m);
      m_hash_stack.push_back(m_position_stack.back().hashes_after(m_hash_stack.back(), m));
      m_position_stack.emplace_back(m_position_stack.back().move(m));
    }

    auto unmove() -> void {
      m_position_stack.pop_back();
      m_move_stack.pop_back();
      m_hash_stack.pop_back();
    }

    auto print_game_record() const -> void;
    auto print_hash_stack() const -> void;
  };

}  // namespace rose
