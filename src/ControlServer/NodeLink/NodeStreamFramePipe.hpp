#pragma once

#include "src/ControlServer/IpcServer/FramePipe.hpp"
#include "src/ControlServer/NodeLink/NodeLink.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace NodeLink {

// NodeStreamFramePipe -- a FramePipe over one tunnel stream of a node link.
//
// The master's IpcSession believes it owns a socket carrying a game instance's
// IPC bytes. On a remote node it owns this instead: whatever the session writes
// is sealed into kFrameInstance frames tagged with the instance id, and whatever
// the node relays back is appended to a buffer the session reads from.
//
// The pipe is deliberately byte-oriented. IpcSession does its own length-prefixed
// framing, so a tunnel frame is just an arbitrary chunk of that byte stream --
// which is what lets a remote instance be served by the exact same dispatcher,
// with no message-level changes anywhere.
class NodeStreamFramePipe : public FramePipe,
                            public std::enable_shared_from_this<NodeStreamFramePipe> {
public:
    // Backstop on relayed bytes that no reader has consumed. IpcSession always
    // has a read outstanding, so this only fills if the session is wedged; at
    // that point dropping the stream beats growing the heap.
    static constexpr std::size_t kMaxBufferedBytes = 8u << 20;

    NodeStreamFramePipe(std::shared_ptr<Link> link, std::int64_t instance_id,
                        std::string node_name);

    // FramePipe.
    void ReadSome(void* data, std::size_t len, FramePipeIoHandler done) override;
    void WriteSome(std::string data, FramePipeIoHandler done) override;
    void Close() override;
    asio::any_io_executor GetExecutor() override;
    // Always false: the admin path gates on this, and a node operator must never
    // be able to issue admin actions through an instance tunnel.
    bool IsLocalPeer() const override { return false; }
    std::string PeerLabel() const override;

    // Append bytes relayed from the node and complete a parked read if it can now
    // be satisfied. Called from the link's frame handler.
    void Feed(std::string chunk);

    std::int64_t InstanceId() const { return instance_id_; }
    std::size_t  BufferedBytes() const { return in_.size(); }
    bool IsClosed() const { return closed_; }

private:
    void try_complete_read();
    void fail(asio::error_code ec);

    std::shared_ptr<Link> link_;
    std::int64_t instance_id_ = 0;
    std::string  node_name_;

    std::string in_;                 // received, not yet consumed
    void*       pending_buf_ = nullptr;
    std::size_t pending_len_ = 0;
    FramePipeIoHandler pending_done_;

    bool closed_ = false;
};

}  // namespace NodeLink
