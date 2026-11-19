#include "src/ControlServer/NodeLink/NodeHandshake.hpp"
#include "src/ControlServer/NodeLink/FrameIo.hpp"
#include "src/ControlServer/Logger.hpp"
#include "lib/nlohmann/json.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <utility>

namespace NodeLink {
namespace {

// Reads [u32 len][json], writes the same, and owns the state machine for one
// role. Kept alive by every in-flight async operation capturing shared_from_this.
class Driver : public std::enable_shared_from_this<Driver> {
public:
    Driver(asio::ip::tcp::socket socket, HandshakeDone done, HandshakeFail fail)
        : socket_(std::move(socket)), done_(std::move(done)), fail_(std::move(fail)) {
        asio::error_code ec;
        const auto remote = socket_.remote_endpoint(ec);
        peer_label_ = ec ? std::string("unknown")
                         : remote.address().to_string() + ":" + std::to_string(remote.port());
    }

    const std::string& PeerLabel() const { return peer_label_; }

    void StartResponder(PskLookup lookup) {
        responder_ = std::make_unique<NodeCrypto::Responder>();
        arm_timeout();

        read_plain([this, lookup = std::move(lookup)](std::string hello) {
            std::string node_id, err;
            if (!responder_->OnHello(hello, node_id, err)) {
                abort(err);
                return;
            }
            NodeCrypto::Key psk{};
            if (!lookup(node_id, psk)) {
                // Hardening: an unknown id never reaches the crypto.
                abort("unknown node_id '" + node_id + "'");
                return;
            }
            node_id_ = node_id;

            std::string challenge = responder_->BuildChallenge(psk, err);
            if (challenge.empty()) {
                abort(err.empty() ? "could not build challenge" : err);
                return;
            }
            write_plain(challenge, [this]() {
                read_plain([this](std::string auth) {
                    std::string auth_ok, err;
                    if (!responder_->OnAuth(auth, auth_ok, session_, err)) {
                        abort(err);
                        return;
                    }
                    // Send the master's proof before handing over, so the peer
                    // never sees a half-finished handshake.
                    write_plain(auth_ok, [this]() { finish(); });
                });
            });
        });
    }

    void StartInitiator(std::string node_id, const NodeCrypto::Key& psk) {
        node_id_ = node_id;
        initiator_ = std::make_unique<NodeCrypto::Initiator>(node_id, psk);
        arm_timeout();

        const std::string hello = initiator_->Start();
        if (hello.empty()) {
            abort("could not build NODE_HELLO");
            return;
        }
        write_plain(hello, [this]() {
            read_plain([this](std::string challenge) {
                std::string err;
                const std::string auth = initiator_->OnChallenge(challenge, err);
                if (auth.empty()) {
                    abort(err.empty() ? "bad challenge" : err);
                    return;
                }
                write_plain(auth, [this]() {
                    read_plain([this](std::string auth_ok) {
                        std::string err;
                        if (!initiator_->OnAuthOk(auth_ok, session_, err)) {
                            abort(err);
                            return;
                        }
                        finish();
                    });
                });
            });
        });
    }

private:
    void arm_timeout() {
        timeout_ = std::make_unique<asio::steady_timer>(socket_.get_executor());
        timeout_->expires_after(std::chrono::seconds(kHandshakeTimeoutSeconds));
        auto self = shared_from_this();
        timeout_->async_wait([this, self](const asio::error_code& ec) {
            if (ec) return;  // cancelled
            abort("handshake timed out");
        });
    }

    void read_plain(std::function<void(std::string)> on_ok) {
        auto self = shared_from_this();
        asio::async_read(
            socket_, asio::buffer(hdr_, 4), asio::transfer_exactly(4),
            [this, self, on_ok = std::move(on_ok)](const asio::error_code& ec, std::size_t) mutable {
                if (ec) {
                    abort("handshake read: " + ec.message());
                    return;
                }
                const std::uint32_t len = NodeLinkIo::DecodeLength(hdr_.data());
                if (len == 0 || len > kMaxHandshakeFrame) {
                    abort("bad handshake frame length " + std::to_string(len));
                    return;
                }
                auto body = std::make_shared<std::string>();
                body->resize(len);
                auto self2 = shared_from_this();
                asio::async_read(
                    socket_, asio::buffer(*body), asio::transfer_exactly(len),
                    [this, self2, body, on_ok = std::move(on_ok)](
                        const asio::error_code& ec2, std::size_t) mutable {
                        if (ec2) {
                            abort("handshake read: " + ec2.message());
                            return;
                        }
                        on_ok(*body);
                    });
            });
    }

    void write_plain(const std::string& payload, std::function<void()> then) {
        auto frame = std::make_shared<std::string>();
        frame->resize(4 + payload.size());
        NodeLinkIo::EncodeLength(static_cast<std::uint32_t>(payload.size()),
                                reinterpret_cast<std::uint8_t*>(&(*frame)[0]));
        frame->replace(4, payload.size(), payload);

        auto self = shared_from_this();
        asio::async_write(
            socket_, asio::buffer(*frame),
            [this, self, frame, then = std::move(then)](
                const asio::error_code& ec, std::size_t) mutable {
                if (ec) {
                    abort("handshake write: " + ec.message());
                    return;
                }
                if (then) then();
            });
    }

    // Hand the socket and the agreed session to the caller.
    void finish() {
        if (finished_) return;
        finished_ = true;
        cancel_timeout();

        Logger::Log("node", "[NodeLink] handshake with %s complete (node_id='%s')\n",
            peer_label_.c_str(), node_id_.c_str());

        auto done = std::move(done_);
        done_ = nullptr;
        if (!done) return;
        done(std::make_unique<HandshakeResult>(
            std::move(socket_), session_, node_id_, peer_label_));
    }

    // Tell the peer why (best effort) and close.
    void abort(const std::string& reason) {
        if (finished_) return;
        finished_ = true;
        cancel_timeout();

        Logger::Log("node", "[NodeLink] handshake with %s failed: %s\n",
            peer_label_.c_str(), reason.c_str());

        auto fail_cb = std::move(fail_);
        fail_ = nullptr;
        if (fail_cb) fail_cb(reason);

        nlohmann::json j;
        j["type"] = "NODE_ERROR";
        j["text"] = reason;
        const std::string payload = j.dump();

        auto frame = std::make_shared<std::string>();
        frame->resize(4 + payload.size());
        NodeLinkIo::EncodeLength(static_cast<std::uint32_t>(payload.size()),
                                reinterpret_cast<std::uint8_t*>(&(*frame)[0]));
        frame->replace(4, payload.size(), payload);

        auto self = shared_from_this();
        asio::async_write(
            socket_, asio::buffer(*frame),
            [self, frame](const asio::error_code&, std::size_t) {
                asio::error_code ignored;
                self->socket_.close(ignored);
            });
    }

    void cancel_timeout() {
        if (!timeout_) return;
        // basic_waitable_timer::cancel() has no error_code overload; it only
        // throws if the service is broken, which is not a condition we can act on.
        timeout_->cancel();
        timeout_.reset();
    }

    asio::ip::tcp::socket socket_;
    std::string           peer_label_;
    std::array<std::uint8_t, 4> hdr_{};
    bool                  finished_ = false;

    std::string node_id_;
    NodeCrypto::Session session_;
    std::unique_ptr<NodeCrypto::Initiator> initiator_;
    std::unique_ptr<NodeCrypto::Responder> responder_;
    std::unique_ptr<asio::steady_timer>    timeout_;

    HandshakeDone done_;
    HandshakeFail fail_;
};

}  // namespace

void RunResponder(asio::ip::tcp::socket socket, PskLookup psk_for_node,
                  HandshakeDone done, HandshakeFail fail) {
    auto driver = std::make_shared<Driver>(std::move(socket), std::move(done), std::move(fail));
    driver->StartResponder(std::move(psk_for_node));
}

void RunInitiator(asio::ip::tcp::socket socket, std::string node_id,
                  NodeCrypto::Key psk, HandshakeDone done, HandshakeFail fail) {
    auto driver = std::make_shared<Driver>(std::move(socket), std::move(done), std::move(fail));
    driver->StartInitiator(std::move(node_id), psk);
}

}  // namespace NodeLink
