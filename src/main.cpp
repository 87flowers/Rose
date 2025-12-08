#include "rose/version.hpp"

#include <fmt/format.h>

using namespace rose;

auto main() -> int {
  fmt::print("# 🌹 Rose {}{}{}\n", version::version_string, version::git_commit_desc.empty() ? "" : "-", version::git_commit_desc);
  return 0;
}
