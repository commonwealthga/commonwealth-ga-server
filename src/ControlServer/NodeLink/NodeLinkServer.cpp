#include "src/ControlServer/NodeLink/NodeLinkServer.hpp"
#include "src/ControlServer/Logger.hpp"

#include <utility>

namespace NodeLink {

Server::Server(asio::io_context& io, std::uint16_t port, const std::string& bind_addr)
    : io_(io), port_(port), bind_addr_(bind_addr) {}

bool Server::Start(PskLookup psk_for_node, AcceptedHandler on_accepted, FailHandler on_fail) {
    psk_for_node_ = std::move(psk_for_node);
    on_accepted_  = std::move(on_accepted);
    on_fail_      = std::move(on_fail);

    asio::error_code ec;
    acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(io_.get_executor());

    asio::ip::address bind_address = asio::ip::make_address(bind_addr_, ec);
    if (ec) {
        Logger::Log("node", "[NodeLink] Invalid node_bind '%s': %s\n",
            bind_addr_.c_str(), ec.message().c_str());
        return false;
    }
    asio::ip::tcp::endpoint endpoint(bind_address, port_);

    acceptor_->open(endpoint.protocol(), ec);
    if (ec) {
        Logger::Log("node", "[NodeLink] Open failed on port %d: %s\n",
            (int)port_, ec.message().c_str());
        return false;
    }
    acceptor_->set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        Logger::Log("node", "[NodeLink] set_option(reuse_address) failed: %s\n",
            ec.message().c_str());
        acceptor_->close();
        return false;
    }
    acceptor_->bind(endpoint, ec);
    if (ec) {
        Logger::Log("node", "[NodeLink] Bind failed on %s:%d: %s\n",
            bind_addr_.c_str(), (int)port_, ec.message().c_str());
        acceptor_->close();
        return false;
    }
    acceptor_->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        Logger::Log("node", "[NodeLink] Listen failed on %s:%d: %s\n",
            bind_addr_.c_str(), (int)port_, ec.message().c_str());
        acceptor_->close();
        return false;
    }

    listening_ = true;
    Logger::Log("node", "[NodeLink] Listening for nodes on %s:%d\n",
        bind_addr_.c_str(), (int)port_);
    do_accept();
    return true;
}

void Server::Stop() {
    listening_ = false;
    if (acceptor_) {
        asio::error_code ignored;
        acceptor_->close(ignored);
    }
}

std::uint16_t Server::BoundPort() const {
    if (!listening_ || !acceptor_) return 0;
    asio::error_code ec;
    const auto local = acceptor_->local_endpoint(ec);
    if (ec) return 0;
    return local.port();
}

void Server::do_accept() {
    if (!listening_ || !acceptor_) return;

    auto self = shared_from_this();
    acceptor_->async_accept([this, self](const asio::error_code& ec, asio::ip::tcp::socket sock) {
        if (!ec) {
            asio::error_code ep_ec;
            const auto remote = sock.remote_endpoint(ep_ec);
            const std::string peer_ip = ep_ec ? std::string("unknown")
                                              : remote.address().to_string();
            if (!admit(peer_ip)) {
                Logger::Log("node",
                    "[NodeLink] refusing connection from %s: too many pending handshakes "
                    "(%d total)\n", peer_ip.c_str(), pending_total_);
                asio::error_code ignored;
                sock.close(ignored);
            } else {
                RunResponder(
                    std::move(sock), psk_for_node_,
                    [this, self, peer_ip](std::unique_ptr<HandshakeResult> result) {
                        handle_accepted(peer_ip, std::move(result));
                    },
                    [this, self, peer_ip](const std::string& reason) {
                        handle_failed(peer_ip, reason);
                    });
            }
        }
        if (listening_) do_accept();
    });
}

void Server::handle_accepted(std::string peer_ip, std::unique_ptr<HandshakeResult> result) {
    release(peer_ip);
    if (!result) return;
    if (!on_accepted_) return;

    auto link = std::make_shared<Link>(
        std::move(result->socket), result->session, result->peer_label);
    link->SetNodeId(result->node_id);

    Logger::Log("node", "[NodeLink] node '%s' authenticated from %s\n",
        result->node_id.c_str(), result->peer_label.c_str());

    on_accepted_(std::move(link));
}

void Server::handle_failed(std::string peer_ip, const std::string& reason) {
    release(peer_ip);
    if (on_fail_) on_fail_(peer_ip, reason);
}

bool Server::admit(const std::string& peer_ip) {
    if (pending_total_ >= kMaxPendingTotal) return false;
    int& n = pending_by_ip_[peer_ip];
    if (n >= kMaxPendingPerIp) return false;
    ++n;
    ++pending_total_;
    return true;
}

void Server::release(const std::string& peer_ip) {
    if (pending_total_ > 0) --pending_total_;
    auto it = pending_by_ip_.find(peer_ip);
    if (it == pending_by_ip_.end()) return;
    if (--it->second <= 0) pending_by_ip_.erase(it);
}

}  // namespace NodeLink
