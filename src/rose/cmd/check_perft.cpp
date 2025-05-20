#include "rose/cmd/check_perft.h"

#include <cctype>
#include <fstream>
#include <print>
#include <string_view>

#include "rose/cmd/perft.h"
#include "rose/position.h"
#include "rose/util/string.h"
#include "rose/util/tokenizer.h"

namespace rose::check_perft {

  auto run(std::string_view epd_filename) -> void {
    std::ifstream file{std::string{epd_filename}};
    if (!file.is_open()) {
      std::print("error (check_perft): unable to open file '{}'\n", epd_filename);
      return;
    }

    std::string line;
    while (std::getline(file, line)) {
      // Some EPD files have trailing null bytes on lines...
      line.erase(line.find_last_not_of('\0') + 1);

      // Ignore empty lines
      if (std::all_of(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c); }))
        continue;

      const auto split = stringSplit(line, ';');
      if (split.size() <= 1) {
        std::print("error (check_perft): no semicolon on line: '{}'\n", line);
        continue;
      }

      const auto position = Position::parse(split[0]);
      if (!position) {
        std::print("error (check_perft): invalid fen on line: '{}'\n", line);
        continue;
      }

      for (usize i = 1; i < split.size(); i++) {
        Tokenizer it{split[i]};
        const auto depth_str = it.next().substr(1);
        const auto depth = parseU32(depth_str);
        const auto res_str = it.next();
        const auto expected_result = parseU64(res_str);

        if (!depth || !expected_result) {
          std::print("error (check_perft): invalid depth entry '{}' on line: '{}'\n", split[i], line);
          continue;
        }

        std::print("{} D{}          \r", *position, *depth);
        std::fflush(stdout);

        const usize result = perft::value(*position, *depth);

        if (result != *expected_result) {
          std::print("fen:      {}\n", *position);
          std::print("depth:    {}\n", *depth);
          std::print("expected: {}\n", *expected_result);
          std::print("rose:     {}\n", result);
          return;
        }
      }
    }

    std::print("\ndone.\n");
  }

} // namespace rose::check_perft
