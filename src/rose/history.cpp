#include "rose/history.h"

#include <algorithm>
#include <cstring>

#include "rose/tunable.h"
#include "rose/util/assert.h"

namespace rose {

  auto History::clear() -> void { std::memset(&m_quiet_sd, 0, sizeof(m_quiet_sd)); }

  auto History::updateQuietHistory(const Position &position, i32 sign, Move m, i32 depth) -> void {
    rose_assert(sign == +1 || sign == -1);
    rose_assert(!m.capture());

    constexpr i32 k = tunable::history_bonus_scale;
    constexpr i32 c = tunable::history_bonus_const;
    const i32 bonus = std::min(depth * k + c, tunable::history_bonus_max);

    i16 &h = m_quiet_sd[m.from(position).raw][m.to().raw];
    h += sign * bonus - h * bonus / tunable::history_max;
  }

  auto History::getHistory(const Position &position, Move m) const -> i32 { return m_quiet_sd[m.from(position).raw][m.to().raw]; }

} // namespace rose
