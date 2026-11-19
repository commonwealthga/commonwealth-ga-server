#pragma once

#include "src/ControlServer/NodeLink/NodeCrypto.hpp"

#include <asio.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>

// NodeLink.hpp -- one authenticated, encrypted master <-> node connection.
//
// Both roles use this after the handshake (see NodeHandshake.hpp): the master's
// Server accepts and hands out Links, the node's Client owns exactly one. A Link
// owns its socket, its crypto session, the read loop and the write queue.
//
// Threading: everything runs on the io_context thread that created the socket.
// SendFrame/Close must be called from that thread. There is no locking on
// purpose -- the control server already serialises on one io thread, and an IPC
// relay that hopped threads would add latency for nothing.

namespace NodeLink {

// What the payload means. The byte rides inside the sealed frame, so it is
// authenticated, and the receiver dispatches on it.
enum FrameKind : std::uint8_t {
    kFrameControl  = 0,  // JSON control message; stream is always 0
    kFrameInstance = 1,  // raw game-instance IPC bytes; stream = instance_id
};

// Hard cap on one sealed frame body ([mac][cipher], excluding the 4-byte length
// prefix). The prefix is attacker-controlled before authentication, so it is
// never trusted: whatever it claims is checked against this before allocating.
constexpr std::size_t kMaxFrameSize = 1u << 20;  // 1 MiB

// Cap on queued outbound bytes. A peer that stops reading must not be able to
// grow our heap; exceeding this drops the link instead.
constexpr std::size_t kMaxQueuedBytes = 8u << 20;  // 8 MiB

class Link : public std::enable_shared_from_this<Link> {
public:
    // payload is moved, so a large instance frame costs one copy at most.
    using FrameHandler = std::function<void(std::uint8_t kind, std::uint64_t stream,
                                            std::string payload)>;
    using CloseHandler = std::function<void(const std::string& reason)>;

    Link(asio::ip::tcp::socket socket, const NodeCrypto::Session& session,
         std::string peer_label);

    // Start delivering frames. on_close fires exactly once, from Close() or from
    // the first transport/crypto failure.
    void Start(FrameHandler on_frame, CloseHandler on_close);

    // Encrypt, queue and send. Returns false if the link is closed, the session
    // cannot seal, or the queue is full -- in every one of those cases the link
    // is closed by the time this returns.
    bool SendFrame(std::uint8_t kind, std::uint64_t stream, const std::string& payload);

    // Idempotent. Runs on_close the first time.
    void Close(const std::string& reason);

    bool IsOpen() const { return open_; }
    asio::any_io_executor GetExecutor() { return socket_.get_executor(); }
    const std::string& PeerLabel() const { return peer_label_; }
    std::size_t QueuedBytes() const { return queued_bytes_; }

    // Set by the master once the handshake identified the node.
    void SetNodeId(std::string id) { node_id_ = std::move(id); }
    const std::string& NodeId() const { return node_id_; }

private:
    void do_read_header();
    void do_read_body(std::uint32_t len);
    void do_write();
    void fail(const std::string& reason);

    asio::ip::tcp::socket socket_;
    NodeCrypto::Session   session_;
    std::string           peer_label_;
    std::string           node_id_;

    std::array<std::uint8_t, 4> header_{};
    std::string body_;
    std::deque<std::string> write_queue_;
    std::size_t queued_bytes_ = 0;
    bool write_in_progress_ = false;
    bool open_ = false;
    bool started_ = false;

    FrameHandler on_frame_;
    CloseHandler on_close_;
};

}  // namespace NodeLink
