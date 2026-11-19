# Standalone integration tests for the master <-> node transport (NodeLink).
# Real loopback sockets, real handshake, real frames. Links only the transport,
# the crypto and vendored Monocypher -- NOT the control server.
# Run from repo root:  make -C tests/node_link run

REPO_ROOT := ../..
CXX       := g++
CC        := gcc
ASIO_INC  := $(REPO_ROOT)/lib/asio-1.34.2/include
LINK_SRC  := $(REPO_ROOT)/src/ControlServer/NodeLink
CXXFLAGS  := -std=c++17 -pthread -I$(REPO_ROOT) -I$(ASIO_INC) \
             -I$(REPO_ROOT)/lib/monocypher -DASIO_STANDALONE -O0 -g -Wall -Wextra
CFLAGS    := -std=c99 -O0 -g -Wall

OBJS := \
	build/monocypher.o \
	build/node_crypto.o \
	build/node_link.o \
	build/node_handshake.o \
	build/node_link_server.o \
	build/node_link_client.o \
	build/test_node_link.o

all: run

build:
	@mkdir -p build

build/monocypher.o: $(REPO_ROOT)/lib/monocypher/monocypher.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/node_crypto.o: $(LINK_SRC)/NodeCrypto.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/node_link.o: $(LINK_SRC)/NodeLink.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/node_handshake.o: $(LINK_SRC)/NodeHandshake.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/node_link_server.o: $(LINK_SRC)/NodeLinkServer.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/node_link_client.o: $(LINK_SRC)/NodeLinkClient.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/test_node_link.o: test_node_link.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/test_node_link: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -ldl

run: build/test_node_link
	./build/test_node_link

clean:
	rm -rf build

.PHONY: all run clean
