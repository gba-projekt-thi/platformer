# -----------------------------------------------------------------------------
# Host unit tests
#
# Builds and runs the pure-logic tests in this directory with the system's
# own g++/clang++ - deliberately NOT the ARM cross-compiler, and nothing
# here touches Butano. Covers the save-system and game-state logic that
# has zero GBA-hardware dependency (see the top-of-file comments in
# save_integrity.h, save_buffer.h, game_state.h, frame_time.h).
#
# Usage:
#   cd tests/host && make            # build + run
#   cd tests/host && make clean
#
# Include paths mirror the real Makefile's INCLUDES list (bare #include
# filenames, one -I per directory - see the root Makefile/README) so these
# tests always resolve headers the same way the GBA build does.
# -----------------------------------------------------------------------------

CXX      ?= g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O1

ROOT := ../..

INCLUDES := \
    $(ROOT)/game/include \
    $(ROOT)/game/include/player \
    $(ROOT)/game/include/traps \
    $(ROOT)/game/include/level \
    $(ROOT)/game/include/scene \
    $(ROOT)/extern/engine/core/include \
    $(ROOT)/extern/engine/src/save_system/include

INCLUDE_FLAGS := $(addprefix -I,$(INCLUDES)) -I.

TEST_SRC := $(wildcard *.cpp)
ENGINE_SRC := $(ROOT)/extern/engine/src/save_system/src/save_integrity.cpp

BIN := host_tests

.PHONY: test run clean

test: run

run: $(BIN)
	./$(BIN)

$(BIN): $(TEST_SRC) $(ENGINE_SRC) test_framework.h
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) $(TEST_SRC) $(ENGINE_SRC) -o $(BIN)

clean:
	rm -f $(BIN)