#include "rose/version.hpp"

#include <print>

using namespace rose;

auto main() -> int {
  std::print("# 🌹 Rose {}{}{}\n", version::version_string, version::git_commit_desc.empty() ? "" : "-", version::git_commit_desc);
  return 0;
}
