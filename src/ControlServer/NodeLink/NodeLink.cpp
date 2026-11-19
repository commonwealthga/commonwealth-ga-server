#include "src/ControlServer/NodeLink/NodeLink.hpp"
#include "src/ControlServer/NodeLink/FrameIo.hpp"
#include "src/ControlServer/Logger.hpp"

#include <utility>

namespace NodeLink {

Link::Link(asio::ip::tcp::socket socket, const NodeCrypto::Session& session,
           std::string peer_label)
    : socket_(std::move(socket)), session_(session), peer_label_(std::move(peer_label)) {}

void Link::Start(FrameHandler on_frame, CloseHandler on_close) {
    if (started_ || open_) return;
    started_ = true;
    on_frame_ = std::move(on_frame);
    on_close_ = std::move(on_close);
    open_ = true;
    do_read_header();
}

void Link::do_read_header() {
    if (!open_) return;
    auto self = shared_from_this();
    asio::async_read(
        socket_, asio::buffer(header_, 4), asio::transfer_exactly(4),
        [this, self](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                fail("read header: " + ec.message());
                return;
            }
            const std::uint32_t len = NodeLinkIo::DecodeLength(header_.data());
            // Below the minimum the frame cannot even hold kind+stream+mac.
            if (len < NodeCrypto::kMacSize + 9) {
                fail("frame too short");
                return;
            }
            if (len > kMaxFrameSize) {
                fail("frame too large (" + std::to_string(len) + " bytes)");
                return;
            }
            do_read_body(len);
        });
}

void Link::do_read_body(std::uint32_t len) {
    if (!open_) return;
    body_.resize(len);
    auto self = shared_from_this();
    asio::async_read(
        socket_, asio::buffer(body_), asio::transfer_exactly(len),
        [this, self](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                fail("read body: " + ec.message());
                return;
            }
            std::uint8_t  kind = 0;
            std::uint64_t stream = 0;
            std::string   payload;
            std::string   err;
            if (!session_.Open(body_, kind, stream, payload, err)) {
                // Fatal by contract: the sender already advanced its counter, so
                // there is no way back to a shared stream position.
                fail("frame rejected: " + err);
                return;
            }
            if (on_frame_) {
                on_frame_(kind, stream, std::move(payload));
            }
            // The handler may have closed us.
            if (open_) do_read_header();
        });
}

bool Link::SendFrame(std::uint8_t kind, std::uint64_t stream, const std::string& payload) {
    if (!open_) return false;

    // Refuse oversized frames here rather than sealing one the peer is required
    // to drop. The link stays open: this is our caller's bug, not the peer's.
    const std::size_t sealed_len = 9 + payload.size() + NodeCrypto::kMacSize;
    if (sealed_len > kMaxFrameSize) {
        Logger::Log("node",
            "[NodeLink] refusing oversized frame to %s (stream=%llu, %zu bytes > %zu)\n",
            peer_label_.c_str(), (unsigned long long)stream, sealed_len, kMaxFrameSize);
        return false;
    }

    const std::string sealed_body = session_.Seal(kind, stream, payload);
    if (sealed_body.empty()) {
        fail("cannot seal frame (session exhausted or invalid)");
        return false;
    }
    if (queued_bytes_ + 4 + sealed_body.size() > kMaxQueuedBytes) {
        fail("outbound queue limit reached");
        return false;
    }

    // Frame it: [u32 len][sealed body]. len is what the reader validates against
    // kMaxFrameSize before allocating.
    std::string frame;
    frame.resize(4 + sealed_body.size());
    NodeLinkIo::EncodeLength(static_cast<std::uint32_t>(sealed_body.size()),
                            reinterpret_cast<std::uint8_t*>(&frame[0]));
    frame.replace(4, sealed_body.size(), sealed_body);

    queued_bytes_ += frame.size();
    write_queue_.push_back(std::move(frame));
    if (!write_in_progress_) {
        write_in_progress_ = true;
        do_write();
    }
    return true;
}

void Link::do_write() {
    if (!open_ || write_queue_.empty()) {
        write_in_progress_ = false;
        return;
    }

    // The shared_ptr keeps the buffer alive across the async write; it is
    // released when the handler is destroyed.
    auto frame = std::make_shared<std::string>(std::move(write_queue_.front()));
    write_queue_.pop_front();
    queued_bytes_ -= frame->size();

    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(*frame),
        [this, self, frame](const asio::error_code& ec, std::size_t /*bytes*/) {
            if (ec) {
                fail("write: " + ec.message());
                return;
            }
            do_write();
        });
}

void Link::Close(const std::string& reason) {
    fail(reason);
}

void Link::fail(const std::string& reason) {
    if (!open_) return;
    open_ = false;

    asio::error_code ignored;
    socket_.close(ignored);
    write_queue_.clear();
    queued_bytes_ = 0;
    write_in_progress_ = false;

    Logger::Log("node",
        "[NodeLink] link %s closed: %s\n",
        peer_label_.c_str(), reason.c_str());

    // Stay alive across the callback: the owner usually drops its reference from
    // inside it.
    auto keep = shared_from_this();
    if (on_close_) on_close_(reason);
}

}  // namespace NodeLink
