cmake_minimum_required(VERSION 3.30)
project(Rose LANGUAGES CXX)

file(STRINGS src/rose_version.txt ROSE_VERSION LIMIT_COUNT 1)

include(CheckIPOSupported)
check_ipo_supported()

enable_testing()
set(CMAKE_SKIP_TEST_ALL_DEPENDENCY FALSE)
function(do_test name)
  add_executable(test_${name} tests/${name}.cpp $<TARGET_OBJECTS:rose-lib>)
  add_test(${name} test_${name})
endfunction()

# Common settings
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
include_directories(src)
add_compile_options(-march=native)
add_link_options(-march=native)
add_compile_definitions(ROSE_VERSION="${ROSE_VERSION}")

set(srcs
  src/rose/byteboard.cpp
  src/rose/byteboard.h
  src/rose/cmd/perft.cpp
  src/rose/cmd/perft.h
  src/rose/common.h
  src/rose/config.cpp
  src/rose/config.h
  src/rose/engine.cpp
  src/rose/engine.h
  src/rose/eval/eval.h
  src/rose/eval/hce.cpp
  src/rose/eval/hce.h
  src/rose/game.cpp
  src/rose/game.h
  src/rose/geometry.h
  src/rose/hash.cpp
  src/rose/hash.h
  src/rose/line.h
  src/rose/move.cpp
  src/rose/move.h
  src/rose/movegen.cpp
  src/rose/movegen.h
  src/rose/pawns.h
  src/rose/position.cpp
  src/rose/position.h
  src/rose/rays.h
  src/rose/search.cpp
  src/rose/search.h
  src/rose/search_control.h
  src/rose/search_stats.h
  src/rose/square.h
  src/rose/uci.cpp
  src/rose/uci.h
  src/rose/util/assert.cpp
  src/rose/util/assert.h
  src/rose/util/bch/bch.h
  src/rose/util/bch/bit.h
  src/rose/util/bch/clmul.h
  src/rose/util/bch/polynomial.h
  src/rose/util/defer.h
  src/rose/util/sign_extend.h
  src/rose/util/static_vector.h
  src/rose/util/tokenizer.h
  src/rose/util/types.h
  src/rose/util/vec.h
)

add_executable(rose src/main.cpp ${srcs})
target_compile_options(rose PUBLIC $<$<CONFIG:Release>:-DROSE_NO_ASSERTS>)

add_library(rose-lib OBJECT ${srcs})
do_test(bch)
do_test(byteboard)
do_test(game)
do_test(geometry)
do_test(movegen)
do_test(position)
