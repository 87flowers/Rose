#include "rose/eval/hce.h"

#include <array>

#include "rose/position.h"
#include "rose/util/types.h"

namespace rose::eval {

  auto hce(const Position &position) -> i32 {
    static constexpr std::array<i32, 8> piece_values{{0, 0, 100, 300, 0, 350, 500, 800}};

    i32 score = 0;
    for (const PieceType white_ptype : position.pieceListType(Color::white).m)
      score += piece_values[white_ptype.toIndex()];
    for (const PieceType black_ptype : position.pieceListType(Color::black).m)
      score -= piece_values[black_ptype.toIndex()];
    return position.activeColor() == Color::white ? score : -score;
  }

} // namespace rose::eval
