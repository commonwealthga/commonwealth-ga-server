// test_node_link.cpp -- integration tests for the master <-> node transport.
//
// Run: make -C tests/node_link run
//
// Unlike tests/node_crypto these are not unit tests of a pure function: a real
// server binds a real loopback port and a real client dials it, so the handshake
// ordering, the async read loop, the write queue and the reconnect path are all
// exercised the way they will be in production.

#include "src/ControlServer/NodeLink/NodeLinkClient.hpp"
#include "src/ControlServer/NodeLink/NodeLinkServer.hpp"

#include <asio.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

void Check(bool cond, const char* what) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::printf("  FAIL  %s\n", what);
    }
}

void Section(const char* name) {
    std::printf("== %s\n", name);
}

// Pump the io_context until pred() is true or the timeout expires. restart() is
// called per slice so this keeps working after the context runs out of work.
template <typename Pred>
bool PumpUntil(asio::io_context& io, Pred pred, int timeout_ms = 5000) {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (true) {
        if (pred()) return true;
        if (std::chrono::steady_clock::now() >= deadline) return false;
        io.restart();
        io.run_for(std::chrono::milliseconds(5));
    }
}

struct Received {
    std::uint8_t  kind = 0;
    std::uint64_t stream = 0;
    std::string   payload;
};

// Everything one scenario needs. Frames are captured rather than processed, so a
// check can assert on exactly what crossed the wire.
struct Harness {
    asio::io_context io;
    std::shared_ptr<NodeLink::Server> server;
    std::shared_ptr<NodeLink::Client> client;
    NodeCrypto::Key server_psk{};

    std::vector<std::shared_ptr<NodeLink::Link>> accepted;
    std::vector<Received>    server_frames;
    std::vector<Received>    client_frames;
    std::vector<std::string> server_errors;
    std::vector<std::string> client_errors;
    int connects = 0;

    bool StartServer(const std::string& expect_node_id) {
        NodeCrypto::RandomBytes(server_psk.data(), server_psk.size());
        server = std::make_shared<NodeLink::Server>(io, /*port=*/0, "127.0.0.1");

        auto psk = server_psk;
        const std::string expect = expect_node_id;
        return server->Start(
            [psk, expect](const std::string& id, NodeCrypto::Key& out) {
                if (id != expect) return false;
                out = psk;
                return true;
            },
            [this](std::shared_ptr<NodeLink::Link> link) {
                accepted.push_back(link);
                link->Start(
                    [this](std::uint8_t kind, std::uint64_t stream, std::string payload) {
                        server_frames.push_back(Received{kind, stream, std::move(payload)});
                    },
                    [](const std::string&) {});
            },
            [this](const std::string& peer, const std::string& reason) {
                server_errors.push_back(peer + ": " + reason);
            });
    }

    void StartClient(const std::string& node_id, const NodeCrypto::Key& key) {
        client = std::make_shared<NodeLink::Client>(io);
        client->Configure("127.0.0.1", server->BoundPort(), node_id, key);
        client->Start(
            [this](std::uint8_t kind, std::uint64_t stream, std::string payload) {
                client_frames.push_back(Received{kind, stream, std::move(payload)});
            },
            [this](bool connected, const std::string& detail) {
                if (connected) ++connects;
                else client_errors.push_back(detail);
            });
    }
};

void TestHandshakeAndFrames() {
    Section("server authenticates the node and frames round-trip");
    Harness h;
    Check(h.StartServer("test-node"), "server listening");
    Check(h.server->BoundPort() != 0, "OS assigned a port");
    if (!h.server->IsListening()) return;

    h.StartClient("test-node", h.server_psk);
    Check(PumpUntil(h.io, [&] { return !h.accepted.empty() && h.client->IsConnected(); }),
          "client connected and server accepted");
    if (h.accepted.empty()) return;

    Check(h.accepted[0]->NodeId() == "test-node", "server learned the node id");
    std::printf("  connected: node='%s' peer=%s\n",
        h.accepted[0]->NodeId().c_str(), h.accepted[0]->PeerLabel().c_str());

    // --- node -> master, control frame ---
    Check(h.client->SendControl("{\"hello\":1}"), "client sent a control frame");
    Check(PumpUntil(h.io, [&] { return !h.server_frames.empty(); }), "server received it");
    if (!h.server_frames.empty()) {
        Check(h.server_frames[0].kind == NodeLink::kFrameControl, "kind is control");
        Check(h.server_frames[0].stream == 0, "control stream is 0");
        Check(h.server_frames[0].payload == "{\"hello\":1}", "payload intact");
    }

    // --- master -> node ---
    Check(h.accepted[0]->SendFrame(NodeLink::kFrameControl, 0, "pong"), "server sent a reply");
    Check(PumpUntil(h.io, [&] { return !h.client_frames.empty(); }), "client received it");
    if (!h.client_frames.empty()) {
        Check(h.client_frames[0].payload == "pong", "reply payload intact");
    }

    // --- instance tunnel frame: non-zero stream, 100 KiB of non-trivial bytes ---
    std::string blob(100 * 1024, '\0');
    for (std::size_t i = 0; i < blob.size(); ++i) {
        blob[i] = static_cast<char>(i & 0xFF);
    }
    Check(h.client->SendInstance(42, blob), "client sent an instance frame");
    Check(PumpUntil(h.io, [&] { return h.server_frames.size() >= 2; }),
          "server received the instance frame");
    if (h.server_frames.size() >= 2) {
        const Received& f = h.server_frames[1];
        Check(f.kind == NodeLink::kFrameInstance, "kind is instance");
        Check(f.stream == 42, "stream carries the instance id");
        Check(f.payload == blob, "100 KiB payload intact");
    }

    // --- an oversized frame is refused locally and does not cost the link ---
    const std::string huge(NodeLink::kMaxFrameSize, 'z');
    Check(!h.client->SendInstance(43, huge), "oversized frame refused");
    Check(h.client->IsConnected(), "link survives the refusal");

    // --- the link still works afterwards ---
    Check(h.client->SendControl("after"), "client still sends");
    Check(PumpUntil(h.io, [&] { return h.server_frames.size() >= 3; }),
          "server still receives");

    h.client->Stop();
}

void TestUnknownNodeRefused() {
    Section("an unknown node id is refused before any crypto");
    Harness h;
    Check(h.StartServer("test-node"), "server listening");
    if (!h.server->IsListening()) return;

    NodeCrypto::Key other{};
    NodeCrypto::RandomBytes(other.data(), other.size());
    h.StartClient("ghost", other);

    Check(PumpUntil(h.io, [&] { return !h.client_errors.empty(); }), "client saw a failure");
    Check(h.accepted.empty(), "no link accepted");
    Check(!h.server_errors.empty(), "server logged the refusal");
    if (!h.server_errors.empty()) std::printf("  server: %s\n", h.server_errors[0].c_str());
    if (!h.client_errors.empty()) std::printf("  client: %s\n", h.client_errors[0].c_str());

    h.client->Stop();
}

void TestWrongKeyRefused() {
    Section("a wrong pre-shared key is refused");
    Harness h;
    Check(h.StartServer("test-node"), "server listening");
    if (!h.server->IsListening()) return;

    NodeCrypto::Key wrong{};
    NodeCrypto::RandomBytes(wrong.data(), wrong.size());
    h.StartClient("test-node", wrong);

    Check(PumpUntil(h.io, [&] { return !h.client_errors.empty(); }), "client saw a failure");
    Check(h.accepted.empty(), "no link accepted");
    if (!h.client_errors.empty()) std::printf("  client: %s\n", h.client_errors[0].c_str());

    h.client->Stop();
}

void TestReconnect() {
    Section("the node reconnects after the master drops the link");
    Harness h;
    Check(h.StartServer("test-node"), "server listening");
    if (!h.server->IsListening()) return;

    h.StartClient("test-node", h.server_psk);
    Check(PumpUntil(h.io, [&] { return h.accepted.size() == 1 && h.client->IsConnected(); }),
          "first connection");
    if (h.accepted.size() != 1) return;

    // Master-side drop, as a restart or an eviction would do.
    h.accepted[0]->Close("test: master dropped the link");
    Check(PumpUntil(h.io, [&] { return !h.client->IsConnected(); }, 3000),
          "client noticed the drop");

    // First reconnect attempt waits 1s (+ jitter), so allow generous slack.
    Check(PumpUntil(h.io, [&] { return h.accepted.size() == 2; }, 10000),
          "client reconnected and re-authenticated a fresh handshake");
    if (h.accepted.size() == 2) {
        Check(h.accepted[1]->NodeId() == "test-node", "second link identifies the node");
        Check(h.client->IsConnected(), "client reports connected again");
        std::printf("  reconnected after %d connect(s)\n", h.connects);
    }

    h.client->Stop();
}

}  // namespace

int main() {
    TestHandshakeAndFrames();
    TestUnknownNodeRefused();
    TestWrongKeyRefused();
    TestReconnect();

    std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
