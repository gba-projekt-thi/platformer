# -----------------------------------------------------------------------------
# Host unit tests
#
# Builds and runs the pure-logic tests in this directory with the system's
# own g++/clang++ - deliberately NOT the ARM cross-compiler. Nothing here
# needs Butano's runtime, only its header-only types (bn::fixed), so the
# rules are tested with the game's own arithmetic. Covers the logic that has
# zero GBA-hardware dependency (see the top-of-file comments in
# save_integrity.h, locomotion.h, trigger_logic.h, ...).
#
# Usage:
#   cd tests/host && make                    # build + run (incremental, parallel)
#   cd tests/host && make FILTER=crumble     # only tests whose name contains it
#   cd tests/host && make coverage           # + line coverage of game/include
#   cd tests/host && make clean
#
# Every test file is its own object (in build/host-tests/, already ignored by
# git) with header dependencies, so a change only recompiles what includes it.
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
    $(ROOT)/extern/engine/src/save_system/include \
    $(ROOT)/extern/engine/extern/butano/butano/include

INCLUDE_FLAGS := $(addprefix -I,$(INCLUDES)) -I.

OBJ_DIR := $(ROOT)/build/host-tests
TEST_SRC := $(wildcard *.cpp)
ENGINE_SRC := $(ROOT)/extern/engine/src/save_system/src/save_integrity.cpp
OBJS := $(addprefix $(OBJ_DIR)/,$(TEST_SRC:.cpp=.o)) $(OBJ_DIR)/save_integrity.o

BIN := host_tests

# Runs the build on every core unless the caller chose -j itself.
ifeq ($(findstring j,$(firstword -$(MAKEFLAGS))),)
MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)
endif

.PHONY: test run coverage clean FORCE

test: run

run: $(BIN)
	./$(BIN) $(FILTER)

# Always relinked (a few ms): a deleted test file leaves every remaining
# object up to date, but its test must leave the binary too.
$(BIN): $(OBJS) FORCE
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN)

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/save_integrity.o: $(ENGINE_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

# Line coverage of the game code the tests exercise, per file (see
# tools/coverage_summary.py; needs only gcov).
COV_DIR := $(ROOT)/build/host-tests-coverage

coverage:
	rm -f $(COV_DIR)/*.gcda
	$(MAKE) OBJ_DIR=$(COV_DIR) BIN=$(COV_DIR)/host_tests \
	    CXXFLAGS="$(CXXFLAGS) -O0 --coverage" run >/dev/null
	python3 $(ROOT)/tools/coverage_summary.py $(COV_DIR) $(ROOT)/game

clean:
	rm -rf $(BIN) $(OBJ_DIR) $(COV_DIR)

-include $(OBJS:.o=.d)
