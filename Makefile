.RECIPEPREFIX = >
SUFFIX :=
ROOT_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

VERSION := $(file < src/rose_version.txt)
GIT_COMMIT_DESC := $(shell git describe --always --dirty)
GIT_COMMIT_HASH := $(shell git show -s --format=%H)

EXE ?= rose
ARCH ?= native

CXX := clang++
CPPFLAGS := -Isrc -MMD -MP
CPPFLAGS += -DFMT_HEADER_ONLY -Ivendor/fmt/include
CPPFLAGS += -Ivendor/lps/include
CXXFLAGS := -std=c++26 -march=$(ARCH) -flto
LDFLAGS  := -pthread -flto
RELFLAGS := -DNDEBUG -O3 -DROSE_NO_ASSERTS -flto=thin
DEBFLAGS := -DNDEBUG -O2 -g

VERSION_FLAGS := -DROSE_VERSION=\"$(VERSION)\"
VERSION_FLAGS += -DROSE_GIT_COMMIT_HASH=\"$(GIT_COMMIT_HASH)\"
VERSION_FLAGS += -DROSE_GIT_COMMIT_DESC=\"$(GIT_COMMIT_DESC)\"

BUILD_DIR := ./build/$(ARCH)

LIB_SRCS := $(wildcard src/rose/*.cpp) $(wildcard src/rose/**/*.cpp)
TOOL_SRCS := $(wildcard tools/*.cpp)
TEST_SRCS := $(wildcard tests/*.cpp)

LIB_REL_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/rel/%.o,$(LIB_SRCS))
LIB_DEB_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/deb/%.o,$(LIB_SRCS))

DEPS := $(LIB_REL_OBJS:.o=.d) $(LIB_DEB_OBJS:.o=.d)

TOOLS := $(patsubst tools/%.cpp,%,$(TOOL_SRCS))
TESTS := $(patsubst tests/%.cpp,$(BUILD_DIR)/%,$(TEST_SRCS))

all: $(EXE) rose-debug $(TOOLS) $(TESTS)

clean:
> rm -r ./build

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

$(TESTS): $(BUILD_DIR)/%: $(BUILD_DIR)/deb/tests/%.o $(LIB_DEB_OBJS)
> $(CXX) $^ -o $@ $(LDFLAGS) $(DEBFLAGS)

$(BUILD_DIR)/rel/%.o: %.cpp
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(RELFLAGS) -c $< -o $@

$(BUILD_DIR)/deb/%.o: %.cpp
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBFLAGS) -c $< -o $@

$(BUILD_DIR)/rel/src/rose/version.o: .FORCE
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(RELFLAGS) $(VERSION_FLAGS) -c src/rose/version.cpp -o $@

$(BUILD_DIR)/deb/src/rose/version.o: .FORCE
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBFLAGS) $(VERSION_FLAGS) -c src/rose/version.cpp -o $@

update-lps:
> @test -z "$(shell git status --porcelain)" || (echo "Working directory not clean" && exit 1)
> rm -r vendor/lps
> git clone git@github.com:/87flowers/lps vendor/lps
> rm -rf vendor/lps/.git

update-fmt:
> @test -z "$(shell git status --porcelain)" || (echo "Working directory not clean" && exit 1)
> rm -r vendor/fmt
> git clone git@github.com:/fmtlib/fmt vendor/fmt
> rm -rf vendor/fmt/.git

.FORCE:

.PHONY: all clean test bench .FORCE

-include $(DEPS)
