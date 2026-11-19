# Standalone unit tests for the master <-> node link crypto (NodeCrypto).
# Native g++ — links only NodeCrypto + vendored Monocypher, NOT the control
# server or the game DLL. Run from repo root:  make -C tests/node_crypto run
#
# This deliberately does NOT touch the project build.

REPO_ROOT := ../..
CXX       := g++
CC        := gcc
CXXFLAGS  := -std=c++17 -I$(REPO_ROOT) -I$(REPO_ROOT)/lib/monocypher -O0 -g -Wall -Wextra
CFLAGS    := -std=c99 -O0 -g -Wall

MONO_OBJ := build/monocypher.o
CRYPTO_OBJ := build/node_crypto.o
TEST_OBJ := build/test_node_crypto.o

all: run

build:
	@mkdir -p build

$(MONO_OBJ): $(REPO_ROOT)/lib/monocypher/monocypher.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(CRYPTO_OBJ): $(REPO_ROOT)/src/ControlServer/NodeLink/NodeCrypto.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_OBJ): test_node_crypto.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/test_node_crypto: $(TEST_OBJ) $(CRYPTO_OBJ) $(MONO_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJ) $(CRYPTO_OBJ) $(MONO_OBJ)

run: build/test_node_crypto
	./build/test_node_crypto

clean:
	rm -rf build

.PHONY: all run clean
