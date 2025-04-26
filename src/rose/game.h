#pragma once

#include <vector>

#include "rose/move.h"
#include "rose/position.h"
#include "rose/util/types.h"

namespace rose {

  struct Game {
  private:
    std::vector<Position> m_position_stack;
    std::vector<Move> m_move_stack;

  public:
    auto reset() -> void;

    auto position() const -> const Position & { return m_position_stack.back(); }

    auto setPositionStartpos() -> void { setPosition(Position::startpos()); }
    auto setPosition(const Position &new_pos) -> void {
      m_position_stack.clear();
      m_move_stack.clear();
      m_position_stack.push_back(new_pos);
    }

    auto move(Move m) -> void {
      m_position_stack.emplace_back(m_position_stack.back().move(m));
      m_move_stack.push_back(m);
    }

    auto unmove() -> void {
      m_position_stack.pop_back();
      m_move_stack.pop_back();
    }

    auto printGameRecord() const -> void;
  };

} // namespace rose
