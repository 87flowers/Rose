.RECIPEPREFIX = >
SUFFIX :=

VERSION := $(file < src/rose_version.txt)
EXE ?= rose-$(VERSION)

GIT_COMMIT_DESC := $(shell git describe --always --dirty)
GIT_COMMIT_HASH := $(shell git show -s --format=%H)

CXX := clang++
CPPFLAGS := -Isrc -MMD -MP
CXXFLAGS := -std=c++26 -march=native -flto=thin -DROSE_VERSION=\"$(VERSION)\" -DROSE_GIT_COMMIT_HASH=\"$(GIT_COMMIT_HASH)\"  -DROSE_GIT_COMMIT_DESC=\"$(GIT_COMMIT_DESC)\"
LDFLAGS  := -flto=thin -pthread
RELFLAGS := -DNDEBUG -O3 -DROSE_NO_ASSERTS
DEBFLAGS := -DNDEBUG -O2 -g

BUILD_DIR := ./build

LIB_SRCS := $(wildcard src/rose/*.cpp) $(wildcard src/rose/**/*.cpp)
TEST_SRCS := $(wildcard tests/*.cpp)

LIB_REL_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/rel/%.o,$(LIB_SRCS))
LIB_DEB_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/deb/%.o,$(LIB_SRCS))

DEPS := $(LIB_REL_OBJS:.o=.d) $(LIB_DEB_OBJS:.o=.d)

TESTS := $(patsubst tests/%.cpp,$(BUILD_DIR)/test_%,$(TEST_SRCS))

all: $(EXE)

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

$(TESTS): $(BUILD_DIR)/test_%: $(BUILD_DIR)/deb/tests/%.o $(LIB_DEB_OBJS)
> $(CXX) $^ -o $@ $(LDFLAGS) $(DEBFLAGS)

$(BUILD_DIR)/rel/%.o: %.cpp
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(RELFLAGS) -c $< -o $@

$(BUILD_DIR)/deb/%.o: %.cpp
> @mkdir -p $(dir $@)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBFLAGS) -c $< -o $@

.PHONY: all clean test bench

-include $(DEPS)
