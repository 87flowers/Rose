#include "rose/nnue/nnue.hpp"

#include "rose/common.hpp"
#include "rose/nnue/network.hpp"
#include "rose/position.hpp"

namespace rose::nnue {

  auto accumulators_from_position(const Position& pos) -> Accumulators {
    const Network& net = default_network();

    Accumulators result;
    result.values.fill(net.accumulator_biases);

    for (u8 i = 0; i < 64; i++) {
      const Square sq {i};
      const Place p = pos.board()[sq];

      if (p.is_empty())
        continue;

      const usize feature0 = feature_index(Color::white, sq, p.ptype(), p.color());
      const usize feature1 = feature_index(Color::black, sq, p.ptype(), p.color());

      add(net, result.values[0], feature0);
      add(net, result.values[1], feature1);
    }
    return result;
  }

  auto evaluate(const Position& pos) -> Score {
    return evaluate(accumulators_from_position(pos), pos.stm());
  }

}  // namespace rose::nnue
