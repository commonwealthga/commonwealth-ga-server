#include "src/ControlServer/NodeAgent/InstanceRelay.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeLink/FrameIo.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"

#include <utility>

namespace {

// The game DLL's IPC frame prefix is the same 4-byte little-endian length the
// node link uses; reusing the helper keeps one implementation of it. The layers
// are still distinct: this prefix delimits instance IPC messages, that one
// delimits tunnel frames.
std::string FrameWith(std::uint32_t len, const std::string& body) {
    std::string frame;
    frame.resize(4 + body.size());
    NodeLinkIo::EncodeLength(len, reinterpret_cast<std::uint8_t*>(&frame[0]));
    frame.replace(4, body.size(), body);
    return frame;
}

}  // namespace

InstanceRelay::InstanceRelay(asio::ip::tcp::socket socket, SendToMaster send,
                             OnAdopted adopted, OnReleased released)
    : socket_(std::move(socket)), send_(std::move(send)),
      adopted_(std::move(adopted)), released_(std::move(released)) {
    asio::error_code ec;
    const auto remote = socket_.remote_endpoint(ec);
    peer_label_ = ec ? std::string("unknown")
                     : remote.address().to_string() + ":" + std::to_string(remote.port());
}

void InstanceRelay::Start() {
    if (started_) return;
    started_ = true;
    open_ = true;
    do_read_header();
}

void InstanceRelay::do_read_header() {
    if (!open_) return;
    auto self = shared_from_this();
    asio::async_read(
        socket_, asio::buffer(header_, 4), asio::transfer_exactly(4),
        [this, self](const asio::error_code& ec, std::size_t) {
            if (ec) {
                fail("read header: " + ec.message());
                return;
            }
            const std::uint32_t len = NodeLinkIo::DecodeLength(header_.data());
            if (len == 0 || len > kMaxFrameSize) {
                fail("bad instance frame length " + std::to_string(len));
                return;
            }
            do_read_body(len);
        });
}

void InstanceRelay::do_read_body(std::uint32_t len) {
    if (!open_) return;
    body_.resize(len);
    auto self = shared_from_this();
    asio::async_read(
        socket_, asio::buffer(body_), asio::transfer_exactly(len),
        [this, self, len](const asio::error_code& ec, std::size_t) {
            if (ec) {
                fail("read body: " + ec.message());
                return;
            }

            if (instance_id_ == 0) {
                handle_pre_hello(len);
                if (open_) do_read_header();
                return;
            }

            if (!forward(len, body_)) return;   // forward() already failed us
            if (open_) do_read_header();
        });
}

void InstanceRelay::handle_pre_hello(std::uint32_t len) {
    auto j = nlohmann::json::parse(body_, nullptr, /*allow_exceptions=*/false,
                                   /*ignore_comments=*/true);
    const std::string type = j.is_object() ? j.value("type", "") : std::string();

    if (type == IpcProtocol::MSG_PING) {
        // The node IS the DLL's IPC endpoint now, so the liveness probe is ours
        // to answer. It also has to be answered here: the master cannot route a
        // frame for an instance it has not been introduced to yet.
        answer_ping();
        return;
    }

    if (type == IpcProtocol::MSG_INSTANCE_HELLO) {
        if (!adopt(body_)) return;   // adopt() already failed us
        // HELLO first so the master registers the session, then whatever was
        // queued ahead of it on this connection.
        if (!forward(len, body_)) return;
        if (!pre_hello_.empty()) {
            if (send_ && !send_(instance_id_, std::move(pre_hello_))) {
                fail("master link refused buffered frames (offline buffer full or closed)");
                return;
            }
            pre_hello_.clear();
        }
        return;
    }

    // Anything else before HELLO is held. The DLL's outbound queue is not cleared
    // when a connection fails, so a message queued during the previous attempt
    // (INSTANCE_READY, a match event) legitimately arrives first.
    if (pre_hello_.size() + 4 + body_.size() > kMaxPreHelloBytes) {
        fail("too many bytes received before INSTANCE_HELLO");
        return;
    }
    pre_hello_ += FrameWith(len, body_);
    Logger::Log("node",
        "[NodeAgent] holding '%s' from %s until INSTANCE_HELLO arrives\n",
        type.empty() ? "<unnamed>" : type.c_str(), peer_label_.c_str());
}

void InstanceRelay::answer_ping() {
    nlohmann::json pong;
    pong["type"] = IpcProtocol::MSG_PONG;
    const std::string body = pong.dump();
    write_to_instance(FrameWith(static_cast<std::uint32_t>(body.size()), body));
}

bool InstanceRelay::forward(std::uint32_t len, const std::string& body) {
    if (send_ && !send_(instance_id_, FrameWith(len, body))) {
        // A link that cannot take this frame is not a link worth keeping: a
        // half-relayed IPC conversation would corrupt the master's view of the
        // instance. The master unwinds it on stream close.
        fail("master link refused a frame (offline buffer full or closed)");
        return false;
    }
    return true;
}

bool InstanceRelay::adopt(const std::string& body) {
    auto j = nlohmann::json::parse(body, nullptr, /*allow_exceptions=*/false,
                                   /*ignore_comments=*/true);
    if (j.is_discarded() || !j.is_object()) {
        fail("first message was not JSON");
        return false;
    }
    const std::string type = j.value("type", "");
    if (type != IpcProtocol::MSG_INSTANCE_HELLO) {
        fail("first message was '" + type + "', expected INSTANCE_HELLO");
        return false;
    }
    const auto id = j.value("instance_id", (std::int64_t)0);
    if (id <= 0) {
        fail("INSTANCE_HELLO carried instance_id=0");
        return false;
    }
    instance_id_ = id;
    Logger::Log("node", "[NodeAgent] instance=%lld connected on %s\n",
        (long long)instance_id_, peer_label_.c_str());
    // Register before the HELLO is forwarded: the master may answer immediately,
    // and that answer has to find this relay.
    if (adopted_) adopted_(instance_id_, shared_from_this());
    return true;
}

void InstanceRelay::Deliver(std::string encoded_frame) {
    write_to_instance(std::move(encoded_frame));
}

void InstanceRelay::write_to_instance(std::string encoded_frame) {
    if (!open_) return;
    write_queue_.push_back(std::move(encoded_frame));
    if (!write_in_progress_) {
        write_in_progress_ = true;
        do_write();
    }
}

void InstanceRelay::do_write() {
    if (!open_ || write_queue_.empty()) {
        write_in_progress_ = false;
        return;
    }

    auto frame = std::make_shared<std::string>(std::move(write_queue_.front()));
    write_queue_.pop_front();

    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(*frame),
        [this, self, frame](const asio::error_code& ec, std::size_t) {
            if (ec) {
                fail("write: " + ec.message());
                return;
            }
            do_write();
        });
}

void InstanceRelay::Close(const std::string& reason) {
    fail(reason);
}

void InstanceRelay::fail(const std::string& reason) {
    if (!open_) return;
    open_ = false;

    asio::error_code ignored;
    socket_.close(ignored);
    write_queue_.clear();
    write_in_progress_ = false;

    if (instance_id_ != 0) {
        Logger::Log("node", "[NodeAgent] instance=%lld relay closed: %s\n",
            (long long)instance_id_, reason.c_str());
    } else {
        Logger::Log("node", "[NodeAgent] unidentified relay from %s closed: %s\n",
            peer_label_.c_str(), reason.c_str());
    }

    // Stay alive across the callback: the owner drops its reference from inside it.
    auto keep = shared_from_this();
    if (released_) released_(instance_id_);
}
