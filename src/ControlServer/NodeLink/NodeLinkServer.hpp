#pragma once

#include "src/ControlServer/NodeLink/NodeHandshake.hpp"
#include "src/ControlServer/NodeLink/NodeLink.hpp"

#include <asio.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

// NodeLinkServer.hpp -- the master's listener for node connections.
//
// Nodes dial in; this accepts, runs the responder handshake, and hands every
// authenticated Link to the NodeRegistry. Accept is capped per source IP and in
// total so a flood cannot pin unbounded handshake state (each handshake is also
// 15s-bounded, see NodeHandshake.hpp).

namespace NodeLink {

// Ownership: construct through std::make_shared -- the accept loop and every
// in-flight handshake hold a weak reference to the object's lifetime via
// shared_from_this.
class Server : public std::enable_shared_from_this<Server> {
public:
    using AcceptedHandler = std::function<void(std::shared_ptr<Link>)>;
    using FailHandler = std::function<void(const std::string& peer_label,
                                           const std::string& reason)>;

    // Max simultaneous unfinished handshakes from one address / in total.
    static constexpr int kMaxPendingPerIp = 4;
    static constexpr int kMaxPendingTotal = 128;

    Server(asio::io_context& io, std::uint16_t port, const std::string& bind_addr);

    // Binds and starts accepting. Returns false (and logs) if the listener could
    // not be created -- the caller should treat that as fatal at startup, the
    // same way it treats a failed IPC bind.
    bool Start(PskLookup psk_for_node, AcceptedHandler on_accepted, FailHandler on_fail);

    void Stop();

    bool IsListening() const { return listening_; }
    // The configured port.
    std::uint16_t Port() const { return port_; }
    // The port actually bound (differs from Port() when the config used 0 and
    // the OS picked one). 0 if not listening.
    std::uint16_t BoundPort() const;

private:
    void do_accept();
    // Named apart from the handler members below so a call site cannot be
    // mistaken for a member invocation.
    void handle_accepted(std::string peer_ip, std::unique_ptr<HandshakeResult> result);
    void handle_failed(std::string peer_ip, const std::string& reason);
    bool admit(const std::string& peer_ip);
    void release(const std::string& peer_ip);

    asio::io_context& io_;
    std::uint16_t     port_ = 0;
    std::string       bind_addr_;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    bool listening_ = false;

    PskLookup       psk_for_node_;
    AcceptedHandler on_accepted_;
    FailHandler     on_fail_;

    std::map<std::string, int> pending_by_ip_;
    int pending_total_ = 0;
};

}  // namespace NodeLink
