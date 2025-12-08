.RECIPEPREFIX = >
SUFFIX :=
ROOT_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

CPPFLAGS := -Iinclude -Itests
CXXFLAGS := -std=c++20 -march=native -Wall -Wextra -Wpedantic -Werror
LDFLAGS  :=
DEBFLAGS := -DNDEBUG -O2 -g

BUILD_DIR := ./build

TEST_SRCS := $(wildcard tests/*.cpp)

TESTS := $(patsubst tests/%.cpp,$(BUILD_DIR)/%,$(TEST_SRCS))

all: $(TESTS)
> for t in $(TESTS); do echo "Running" $$t && $$t > /dev/null || exit 1; done

clean:
> rm -r $(BUILD_DIR)

$(TESTS): $(BUILD_DIR)/%: $(BUILD_DIR)/tests/%.o
> $(CXX) $^ -o $@ $(LDFLAGS) $(DEBFLAGS)

$(BUILD_DIR)/%.o: %.cpp $(EVALFILE)
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBFLAGS) -c $< -o $@

.PHONY: all clean
