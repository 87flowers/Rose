#pragma once

#include <string_view>

namespace rose::check_perft {

  auto run(std::string_view epd_filename) -> void;

} // namespace rose::check_perft
