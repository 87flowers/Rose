#include <print>
#include <vector>

#include "rose/position.h"
#include "rose/tool/datagen/push.h"
#include "rose/util/types.h"

using namespace rose;

auto main() -> int {
  const Position position = Position::startpos();
  std::vector<u8> buf;

  tool::datagen::pushMarlinformatPosition(buf, position);

  for (int i = 0; i < buf.size(); i++) {
    std::print("{:02x} ", buf[i]);
    if (i % 8 == 7)
      std::print("\n");
  }
  std::print("\n");

  std::vector<u8> expected{
      0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x16, 0x42, 0x25, 0x61, 0x00, 0x00, 0x00, 0x00,
      0x88, 0x88, 0x88, 0x88, 0x9e, 0xca, 0xad, 0xe9, 0x40, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  rose_assert(buf == expected);

  return 0;
}
