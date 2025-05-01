#pragma once

#include <vector>

#include "rose/move.h"
#include "rose/position.h"
#include "rose/util/assert.h"
#include "rose/util/types.h"

namespace rose {

  struct Game {
  private:
    std::vector<Position> m_position_stack;
    std::vector<Move> m_move_stack;
    std::vector<u64> m_hash_stack;
    size_t m_waterline = 0;

  public:
    auto reset() -> void;

    auto position() const -> const Position & { return m_position_stack.back(); }
    auto hash() const -> u64 { return m_hash_stack.back(); }

    auto setPositionStartpos() -> void { setPosition(Position::startpos()); }
    auto setPosition(const Position &new_pos) -> void {
      m_position_stack.clear();
      m_move_stack.clear();
      m_position_stack.push_back(new_pos);
      m_hash_stack.push_back(new_pos.hash());
    }

    auto setWaterline() -> void { m_waterline = m_position_stack.size() - 1; }
    auto assertAtWaterline() -> void { rose_assert(m_waterline == m_position_stack.size() - 1); }

    auto move(Move m) -> void {
      m_position_stack.emplace_back(m_position_stack.back().move(m));
      m_move_stack.push_back(m);
      m_hash_stack.push_back(m_position_stack.back().hash());
    }

    auto unmove() -> void {
      m_position_stack.pop_back();
      m_move_stack.pop_back();
      m_hash_stack.pop_back();
    }

    auto isRepetition() const -> bool;

    auto printGameRecord() const -> void;
  };

} // namespace rose
