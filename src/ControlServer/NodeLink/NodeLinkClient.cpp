#include "src/ControlServer/NodeLink/NodeLinkClient.hpp"
#include "src/ControlServer/Logger.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <utility>

namespace NodeLink {
namespace {

// Jitter source. Only touched from the io thread and only on reconnect, so no
// locking is needed.
int JitterSeconds(int base) {
    static std::mt19937_64 eng{std::random_device{}()};
    if (base <= 1) return base;
    std::uniform_int_distribution<int> dist(0, std::max(1, base / 4));
    return base + dist(eng);
}

}  // namespace

Client::Client(asio::io_context& io)
    : io_(io), reconnect_timer_(io) {}

void Client::Configure(std::string host, std::uint16_t port, std::string node_id,
                       NodeCrypto::Key psk) {
    host_ = std::move(host);
    port_ = port;
    node_id_ = std::move(node_id);
    psk_ = psk;
    configured_ = !host_.empty() && port_ != 0 && !node_id_.empty();
    if (!configured_) {
        Logger::Log("node",
            "[NodeLink] client not configured (master_addr='%s', node_id='%s')\n",
            host_.c_str(), node_id_.c_str());
    }
}

void Client::Start(Link::FrameHandler on_frame, StateHandler on_state) {
    if (running_) return;
    on_frame_ = std::move(on_frame);
    on_state_ = std::move(on_state);

    if (!configured_) {
        set_state(false, "not configured");
        return;
    }
    running_ = true;
    stopping_ = false;
    backoff_secs_ = kBackoffStartSeconds;
    dial();
}

void Client::Stop() {
    stopping_ = true;
    running_ = false;
    // basic_waitable_timer::cancel() has no error_code overload; it only throws
    // if the timer service is broken, which is not something we can recover from.
    reconnect_timer_.cancel();
    if (connect_timeout_) {
        connect_timeout_->cancel();
        connect_timeout_.reset();
    }
    if (resolver_) {
        resolver_->cancel();
        resolver_.reset();
    }
    auto link = link_;
    link_.reset();
    if (link) link->Close("client stopped");
    offline_.clear();
    queued_bytes_ = 0;
}

void Client::dial() {
    if (!running_ || stopping_) return;
    ++attempt_;

    Logger::Log("node", "[NodeLink] connecting to master %s:%d (attempt %llu)\n",
        host_.c_str(), (int)port_, (unsigned long long)attempt_);

    resolver_ = std::make_unique<asio::ip::tcp::resolver>(io_);
    auto self = shared_from_this();
    resolver_->async_resolve(
        host_, std::to_string(port_),
        [this, self](const asio::error_code& ec,
                     asio::ip::tcp::resolver::results_type results) {
            if (stopping_) return;
            if (ec) {
                handle_handshake_failed("resolve: " + ec.message());
                return;
            }

            auto sock = std::make_shared<asio::ip::tcp::socket>(io_);
            // Settled exactly once by whichever of connect/timeout completes
            // first, so the loser cannot act on a socket the winner moved away.
            auto settled = std::make_shared<bool>(false);

            connect_timeout_ = std::make_unique<asio::steady_timer>(io_);
            connect_timeout_->expires_after(std::chrono::seconds(kConnectTimeoutSeconds));
            auto self2 = shared_from_this();
            connect_timeout_->async_wait(
                [this, self2, sock, settled](const asio::error_code& tec) {
                    if (tec) return;       // cancelled: connect finished in time
                    if (*settled) return;  // timer fired late
                    *settled = true;
                    Logger::Log("node", "[NodeLink] connect to master timed out\n");
                    asio::error_code ignored;
                    sock->close(ignored);
                });

            asio::async_connect(
                *sock, results,
                [this, self2, sock, settled](const asio::error_code& cec,
                                             const asio::ip::tcp::endpoint&) {
                    if (*settled) return;  // already timed out
                    *settled = true;
                    if (connect_timeout_) {
                        connect_timeout_->cancel();
                        connect_timeout_.reset();
                    }
                    if (stopping_) return;
                    if (cec) {
                        handle_handshake_failed("connect: " + cec.message());
                        return;
                    }

                    RunInitiator(
                        std::move(*sock), node_id_, psk_,
                        [this, self2](std::unique_ptr<HandshakeResult> result) {
                            handle_connected(std::move(result));
                        },
                        [this, self2](const std::string& reason) {
                            handle_handshake_failed(reason);
                        });
                });
        });
}

void Client::handle_connected(std::unique_ptr<HandshakeResult> result) {
    if (!result || stopping_ || !running_) return;

    link_ = std::make_shared<Link>(std::move(result->socket), result->session,
                                  result->peer_label);

    Logger::Log("node", "[NodeLink] connected to master %s as '%s'\n",
        link_->PeerLabel().c_str(), node_id_.c_str());

    backoff_secs_ = kBackoffStartSeconds;

    auto self = shared_from_this();
    link_->Start(
        [this, self](std::uint8_t kind, std::uint64_t stream, std::string payload) {
            if (on_frame_) on_frame_(kind, stream, std::move(payload));
        },
        [this, self](const std::string& reason) { handle_link_closed(reason); });

    set_state(true, "connected");
    flush_offline();
}

void Client::handle_handshake_failed(const std::string& reason) {
    if (stopping_ || !running_) return;
    Logger::Log("node", "[NodeLink] connect to master failed: %s\n", reason.c_str());
    set_state(false, reason);
    schedule_reconnect();
}

void Client::handle_link_closed(const std::string& reason) {
    link_.reset();
    if (stopping_ || !running_) return;
    Logger::Log("node", "[NodeLink] link to master lost: %s\n", reason.c_str());
    set_state(false, reason);
    schedule_reconnect();
}

void Client::schedule_reconnect() {
    if (!running_ || stopping_) return;

    const int wait = JitterSeconds(backoff_secs_);
    Logger::Log("node", "[NodeLink] reconnecting in %ds\n", wait);

    reconnect_timer_.expires_after(std::chrono::seconds(wait));
    auto self = shared_from_this();
    reconnect_timer_.async_wait([this, self](const asio::error_code& ec) {
        if (ec || stopping_ || !running_) return;
        dial();
    });

    backoff_secs_ = std::min(backoff_secs_ * 2, kBackoffMaxSeconds);
}

bool Client::enqueue(std::uint8_t kind, std::uint64_t stream, std::string payload) {
    if (queued_bytes_ + payload.size() > kMaxOfflineQueueBytes) return false;
    Outbound o;
    o.kind = kind;
    o.stream = stream;
    queued_bytes_ += payload.size();
    o.payload = std::move(payload);
    offline_.push_back(std::move(o));
    return true;
}

bool Client::SendControl(std::string json) {
    if (IsConnected()) {
        return link_->SendFrame(kFrameControl, 0, json);
    }
    return enqueue(kFrameControl, 0, std::move(json));
}

bool Client::SendInstance(std::int64_t instance_id, std::string ipc_bytes) {
    const std::uint64_t stream = static_cast<std::uint64_t>(instance_id);
    if (IsConnected()) {
        return link_->SendFrame(kFrameInstance, stream, ipc_bytes);
    }
    return enqueue(kFrameInstance, stream, std::move(ipc_bytes));
}

void Client::flush_offline() {
    if (!IsConnected()) return;
    if (offline_.empty()) return;

    Logger::Log("node", "[NodeLink] flushing %zu buffered frame(s) (%zu bytes)\n",
        offline_.size(), queued_bytes_);

    while (!offline_.empty() && IsConnected()) {
        Outbound o = std::move(offline_.front());
        offline_.pop_front();
        queued_bytes_ -= o.payload.size();
        // A failed send closes the link; the rest stays queued for the next
        // reconnect, so re-queue it and stop.
        if (!link_->SendFrame(o.kind, o.stream, o.payload)) {
            queued_bytes_ += o.payload.size();
            offline_.push_front(std::move(o));
            return;
        }
    }
}

void Client::set_state(bool connected, const std::string& detail) {
    if (on_state_) on_state_(connected, detail);
}

}  // namespace NodeLink
