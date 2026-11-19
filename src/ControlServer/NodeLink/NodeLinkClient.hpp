#pragma once

#include "src/ControlServer/NodeLink/NodeHandshake.hpp"
#include "src/ControlServer/NodeLink/NodeLink.hpp"

#include <asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>

// NodeLinkClient.hpp -- the node's outbound link to its master.
//
// Dials out, owns the initiator handshake, and reconnects with backoff + jitter
// whenever the link drops. Nodes never listen for management traffic, so a
// master restart is just another reconnect: fresh handshake, fresh keys,
// counters back to zero.
//
// While disconnected, frames are buffered and flushed on the next successful
// handshake. The buffer cap is a backstop, not a policy: what to drop when a
// game instance out-produces the link is the *agent's* decision (it can inspect
// the IPC payload it is relaying). SendInstance/SendControl return false when
// they refuse a frame so the caller can apply its own policy and log it.

namespace NodeLink {

// Ownership: construct through std::make_shared -- dial, reconnect and the link
// callbacks all keep the object alive via shared_from_this.

// Backstop cap on the offline buffer. 16 MiB is minutes of a busy match.
constexpr std::size_t kMaxOfflineQueueBytes = 16u << 20;

// Reconnect backoff: 1s doubling to a 30s cap, plus up to 25% jitter so a
// master restart does not turn into a synchronised stampede from every node.
constexpr int kBackoffStartSeconds = 1;
constexpr int kBackoffMaxSeconds   = 30;

// How long a TCP connect attempt may take before it is abandoned.
constexpr int kConnectTimeoutSeconds = 10;

class Client : public std::enable_shared_from_this<Client> {
public:
    using StateHandler = std::function<void(bool connected, const std::string& detail)>;

    explicit Client(asio::io_context& io);

    // Call once before Start().
    void Configure(std::string host, std::uint16_t port, std::string node_id,
                   NodeCrypto::Key psk);

    // on_frame receives every authenticated frame. on_state fires on each
    // connect/disconnect so the agent can log and gate spawning.
    void Start(Link::FrameHandler on_frame, StateHandler on_state);

    // Stop dialling and close the link. Buffered frames are discarded.
    void Stop();

    bool IsConnected() const { return link_ && link_->IsOpen(); }
    const std::string& NodeId() const { return node_id_; }
    std::size_t QueuedBytes() const { return queued_bytes_; }
    int NextBackoffSeconds() const { return backoff_secs_; }
    std::uint64_t Attempt() const { return attempt_; }

    // stream is 0 for control frames. Both return false when the frame was not
    // accepted (buffer full, or a send failure that just closed the link).
    // Payloads are by value so the caller can move a large instance frame in.
    bool SendControl(std::string json);
    bool SendInstance(std::int64_t instance_id, std::string ipc_bytes);

private:
    struct Outbound {
        std::uint8_t  kind = 0;
        std::uint64_t stream = 0;
        std::string   payload;
    };

    void dial();
    void handle_connected(std::unique_ptr<HandshakeResult> result);
    void handle_handshake_failed(const std::string& reason);
    void handle_link_closed(const std::string& reason);
    void schedule_reconnect();
    void flush_offline();
    bool enqueue(std::uint8_t kind, std::uint64_t stream, std::string payload);
    void set_state(bool connected, const std::string& detail);

    asio::io_context& io_;
    std::string   host_;
    std::uint16_t port_ = 0;
    std::string   node_id_;
    NodeCrypto::Key psk_{};
    bool          configured_ = false;
    bool          running_ = false;
    bool          stopping_ = false;

    std::unique_ptr<asio::ip::tcp::resolver> resolver_;
    std::unique_ptr<asio::steady_timer>      connect_timeout_;
    asio::steady_timer                       reconnect_timer_;
    std::shared_ptr<Link>                    link_;

    std::deque<Outbound>  offline_;
    std::size_t           queued_bytes_ = 0;
    int                   backoff_secs_ = kBackoffStartSeconds;
    std::uint64_t         attempt_ = 0;

    Link::FrameHandler on_frame_;
    StateHandler       on_state_;
};

}  // namespace NodeLink
