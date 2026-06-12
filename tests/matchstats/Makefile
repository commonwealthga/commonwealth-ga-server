# Standalone unit tests for the PURE match-stats stint banking core.
# Native g++ — links only StatsCore, NOT the game DLL. Run from repo
# root:  make -C tests/matchstats run
# This deliberately does NOT touch the project build.

REPO_ROOT := ../..
CXX       := g++
CXXFLAGS  := -std=c++17 -I$(REPO_ROOT) -I. -O0 -g -Wall -Wextra

CORE_SRC := \
    $(REPO_ROOT)/src/GameServer/Stats/StatsCore.cpp

TEST_SRC := \
    main.cpp \
    test_stats_core.cpp

BIN := matchstats_tests

$(BIN): $(CORE_SRC) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_SRC) $(TEST_SRC)

.PHONY: run clean
run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
