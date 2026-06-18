#pragma once

#include "rose/eval/nnue/jasper.hpp"
#include "rose/eval/nnue/kyanite.hpp"

namespace rose::eval::nnue {

  // clang-format off
#define rose_for_each_arch(x)      \
  x(jasper128, Jasper<128>)        \
  x(jasper256, Jasper<256>)        \
  x(kyanite256, Kyanite<256>)      \
  x(kyanite512, Kyanite<512>)      \
  x(kyanite768, Kyanite<768>)
  // clang-format on

}  // namespace rose::eval::nnue
