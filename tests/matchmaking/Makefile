# Standalone unit tests for the PURE matchmaking core (rules + placement).
# Native g++ — links only the dependency-free matchmaking sources, NOT the
# control server or the game DLL. Run from repo root:  make -C tests/matchmaking run
#
# This deliberately does NOT touch the project build (cross-compiled separately).

REPO_ROOT := ../..
CXX       := g++
CXXFLAGS  := -std=c++17 -I$(REPO_ROOT) -I. -O0 -g -Wall -Wextra

CORE_SRC := \
    $(REPO_ROOT)/src/ControlServer/MatchmakingService/SidePlacement.cpp \
    $(REPO_ROOT)/src/ControlServer/MatchmakingService/MmrSwap.cpp \
    $(REPO_ROOT)/src/ControlServer/MatchmakingService/Rules/CoopMatchRule.cpp \
    $(REPO_ROOT)/src/ControlServer/MatchmakingService/Rules/DoubleAgentRule.cpp \
    $(REPO_ROOT)/src/ControlServer/MatchmakingService/Rules/VersusSidesRule.cpp

TEST_SRC := \
    main.cpp \
    test_mmr_swap.cpp \
    test_side_placement.cpp \
    test_coop_rule.cpp \
    test_double_agent_rule.cpp \
    test_versus_sides_rule.cpp

BIN := mm_tests

$(BIN): $(CORE_SRC) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_SRC) $(TEST_SRC)

.PHONY: run clean
run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
