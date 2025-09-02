#include "rose/eval/network.h"

#include "rose/position.h"
#include "rose/util/types.h"

namespace rose::eval {

  alignas(Network) const char g_embedded_network_raw[] = {
#embed ROSE_NETWORK_FILE
  };

  const Network &defaultNetwork() { return *reinterpret_cast<const Network *>(&g_embedded_network_raw[0]); }

  auto accumulatorsFromPosition(const Position &pos) -> Accumulators {
    const Network &net = defaultNetwork();

    Accumulators result;
    result.values.fill(net.accumulator_biases);

    for (u8 i = 0; i < 64; i++) {
      const Square sq{i};
      const Place p = pos.board().read(sq);

      if (p.isEmpty())
        continue;

      const usize feature0 = featureIndex(Color::white, sq, p.ptype(), p.color());
      const usize feature1 = featureIndex(Color::black, sq, p.ptype(), p.color());

      add(net, result.values[0], feature0);
      add(net, result.values[1], feature1);
    }
    return result;
  }

  auto evaluate(const Position &pos) -> i32 { return evaluate(accumulatorsFromPosition(pos), pos.activeColor()); }

} // namespace rose::eval
