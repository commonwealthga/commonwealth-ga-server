#pragma once

#include "src/ControlServer/NodeLink/NodeCrypto.hpp"

#include <asio.hpp>
#include <functional>
#include <memory>
#include <string>

// NodeHandshake.hpp -- the asynchronous driver for the four-message A1 handshake
// on top of a freshly accepted/dialled socket.
//
// Plaintext phase: frames are [u32 len][json]. Nothing is trusted until the peer
// proves it holds the node's PSK, so everything read here is treated as hostile
// input (size-capped, strictly parsed).

namespace NodeLink {

// Cap on a handshake frame. NodeCrypto itself refuses JSON over 8 KiB; this
// bounds the buffer before it is even read.
constexpr std::size_t kMaxHandshakeFrame = 8192;

// How long a peer may take to complete the handshake before we drop it. A
// silent connection holding a socket and half a handshake forever is a resource
// leak an attacker controls.
constexpr int kHandshakeTimeoutSeconds = 15;

// What a completed handshake hands over: the socket it owned, the agreed
// session, and who the peer says it is.
struct HandshakeResult {
    HandshakeResult(asio::ip::tcp::socket s, NodeCrypto::Session sess,
                    std::string id, std::string peer)
        : socket(std::move(s)), session(sess),
          node_id(std::move(id)), peer_label(std::move(peer)) {}

    asio::ip::tcp::socket socket;
    NodeCrypto::Session   session;
    std::string           node_id;      // "" on the initiator side
    std::string           peer_label;
};

using HandshakeDone = std::function<void(std::unique_ptr<HandshakeResult>)>;
using HandshakeFail = std::function<void(const std::string& reason)>;

// Master side. `psk_for_node` resolves node_id -> key from nodes.json; returning
// false refuses the node before any crypto is attempted, which is where unknown
// node ids die.
using PskLookup = std::function<bool(const std::string& node_id, NodeCrypto::Key& out)>;

void RunResponder(asio::ip::tcp::socket socket, PskLookup psk_for_node,
                  HandshakeDone done, HandshakeFail fail);

// Node side.
void RunInitiator(asio::ip::tcp::socket socket, std::string node_id,
                  NodeCrypto::Key psk, HandshakeDone done, HandshakeFail fail);

}  // namespace NodeLink
