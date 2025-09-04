.RECIPEPREFIX = >
SUFFIX :=
ROOT_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

VERSION := $(file < src/rose_version.txt)
DEFAULT_NETWORK := $(file < src/rose_network.txt)
GIT_COMMIT_DESC := $(shell git describe --always --dirty)
GIT_COMMIT_HASH := $(shell git show -s --format=%H)

DEFAULT_NETWORK_FILE := $(ROOT_DIR)/networks/$(DEFAULT_NETWORK).rosenet

EXE ?= rose
EVALFILE ?= $(DEFAULT_NETWORK_FILE)

CXX := clang++
CPPFLAGS := -Isrc -MMD -MP
CXXFLAGS := -std=c++26 -march=native
CXXFLAGS += -DROSE_VERSION=\"$(VERSION)\"
CXXFLAGS += -DROSE_GIT_COMMIT_HASH=\"$(GIT_COMMIT_HASH)\" -DROSE_GIT_COMMIT_DESC=\"$(GIT_COMMIT_DESC)\"
CXXFLAGS += -DROSE_NETWORK_FILE=\"$(EVALFILE)\"
LDFLAGS  := -pthread
RELFLAGS := -DNDEBUG -O3 -DROSE_NO_ASSERTS -flto=thin
DEBFLAGS := -DNDEBUG -O2 -g

BUILD_DIR := ./build

LIB_SRCS := $(wildcard src/rose/*.cpp) $(wildcard src/rose/**/*.cpp)
TOOL_SRCS := $(wildcard tools/*.cpp)
TEST_SRCS := $(wildcard tests/*.cpp)

LIB_REL_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/rel/%.o,$(LIB_SRCS))
LIB_DEB_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/deb/%.o,$(LIB_SRCS))

DEPS := $(LIB_REL_OBJS:.o=.d) $(LIB_DEB_OBJS:.o=.d)

TOOLS := $(patsubst tools/%.cpp,%,$(TOOL_SRCS))
TESTS := $(patsubst tests/%.cpp,$(BUILD_DIR)/test_%,$(TEST_SRCS))

all: $(EXE) $(TOOLS) $(TESTS)

clean:
> rm -r $(BUILD_DIR)

test: $(TESTS)
> for t in $(TESTS); do echo "Running" $$t && $$t > /dev/null || exit 1; done

bench: $(EXE)
> ./$(EXE) bench

$(EXE): $(BUILD_DIR)/rel/src/main.o $(LIB_REL_OBJS)
> $(CXX) $^ -o $@ $(LDFLAGS) $(RELFLAGS)

rose-debug: $(BUILD_DIR)/deb/src/main.o $(LIB_DEB_OBJS)
> $(CXX) $^ -o $@ $(LDFLAGS) $(DEBFLAGS)

$(TOOLS): %: $(BUILD_DIR)/rel/tools/%.o $(LIB_REL_OBJS)
> $(CXX) $^ -o $@ $(LDFLAGS) $(RELFLAGS)

$(TESTS): $(BUILD_DIR)/test_%: $(BUILD_DIR)/deb/tests/%.o $(LIB_DEB_OBJS)
> $(CXX) $^ -o $@ $(LDFLAGS) $(DEBFLAGS)

$(BUILD_DIR)/rel/%.o: %.cpp $(EVALFILE)
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(RELFLAGS) -c $< -o $@

$(BUILD_DIR)/deb/%.o: %.cpp $(EVALFILE)
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBFLAGS) -c $< -o $@

$(DEFAULT_NETWORK_FILE):
> @mkdir -p $(dir $@)
> curl -L https://github.com/87flowers/rose-nets/releases/download/$(DEFAULT_NETWORK)/$(DEFAULT_NETWORK).rosenet -o $(DEFAULT_NETWORK_FILE)

.PHONY: all clean test bench

-include $(DEPS)
