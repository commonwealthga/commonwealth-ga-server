#include "src/ControlServer/NodeLink/NodeStreamFramePipe.hpp"
#include "src/ControlServer/Logger.hpp"

#include <cstring>
#include <utility>

namespace NodeLink {

NodeStreamFramePipe::NodeStreamFramePipe(std::shared_ptr<Link> link,
                                         std::int64_t instance_id,
                                         std::string node_name)
    : link_(std::move(link)), instance_id_(instance_id), node_name_(std::move(node_name)) {}

void NodeStreamFramePipe::ReadSome(void* data, std::size_t len, FramePipeIoHandler done) {
    if (closed_) {
        if (done) {
            auto exec = GetExecutor();
            asio::post(exec, [done = std::move(done)]() {
                done(asio::error::operation_aborted, 0);
            });
        }
        return;
    }

    if (pending_done_) {
        // IpcSession never overlaps reads. Treat a second one as a bug rather
        // than silently dropping the first.
        Logger::Log("node",
            "[NodeStream] instance=%lld: overlapping read requested (%zu bytes) -- refusing\n",
            (long long)instance_id_, len);
        if (done) done(asio::error::already_started, 0);
        return;
    }

    if (len == 0) {
        auto exec = GetExecutor();
        asio::post(exec, [done = std::move(done)]() { done(asio::error_code(), 0); });
        return;
    }

    pending_buf_ = data;
    pending_len_ = len;
    pending_done_ = std::move(done);
    try_complete_read();
}

void NodeStreamFramePipe::WriteSome(std::string data, FramePipeIoHandler done) {
    if (closed_ || !link_) {
        if (done) {
            auto exec = GetExecutor();
            asio::post(exec, [done = std::move(done)]() {
                done(asio::error::operation_aborted, 0);
            });
        }
        return;
    }

    const bool ok = link_->SendFrame(kFrameInstance,
                                     static_cast<std::uint64_t>(instance_id_), data);
    // Complete asynchronously: IpcSession chains its next write from this
    // handler, so completing inline would recurse once per queued frame.
    auto exec = GetExecutor();
    asio::post(exec, [done = std::move(done), ok]() {
        if (!done) return;
        done(ok ? asio::error_code() : asio::error::broken_pipe, 0);
    });
}

void NodeStreamFramePipe::Feed(std::string chunk) {
    if (closed_) return;

    if (in_.size() + chunk.size() > kMaxBufferedBytes) {
        Logger::Log("node",
            "[NodeStream] instance=%lld: %zu bytes buffered with no reader consuming "
            "them -- dropping the stream\n",
            (long long)instance_id_, in_.size());
        Close();
        return;
    }
    in_.append(chunk);
    try_complete_read();
}

void NodeStreamFramePipe::try_complete_read() {
    if (closed_ || !pending_done_) return;
    if (in_.size() < pending_len_) return;

    if (pending_buf_ && pending_len_ > 0) {
        std::memcpy(pending_buf_, in_.data(), pending_len_);
    }
    in_.erase(0, pending_len_);

    auto done = std::move(pending_done_);
    pending_done_ = nullptr;
    pending_buf_ = nullptr;
    pending_len_ = 0;

    auto exec = GetExecutor();
    asio::post(exec, [done = std::move(done)]() {
        if (done) done(asio::error_code(), 0);
    });
}

void NodeStreamFramePipe::Close() {
    if (closed_) return;
    closed_ = true;
    in_.clear();

    if (pending_done_) {
        auto done = std::move(pending_done_);
        pending_done_ = nullptr;
        pending_buf_ = nullptr;
        pending_len_ = 0;
        auto exec = GetExecutor();
        asio::post(exec, [done = std::move(done)]() {
            if (done) done(asio::error::operation_aborted, 0);
        });
    }
}

asio::any_io_executor NodeStreamFramePipe::GetExecutor() {
    // link_ is required by the constructor, so this is always available. It must
    // stay the io thread that owns the link: posting a completion to any other
    // executor would run IpcSession's handler off-thread.
    return link_->GetExecutor();
}

std::string NodeStreamFramePipe::PeerLabel() const {
    return "node:" + node_name_ + "/instance:" + std::to_string(instance_id_);
}

}  // namespace NodeLink
