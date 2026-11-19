#pragma once

#include <asio.hpp>
#include <cstddef>
#include <functional>
#include <string>

// FramePipe.hpp -- the transport an IpcSession speaks over.
//
// IpcSession owns the IPC framing and message dispatch; it owns no socket. That
// separation is what lets a game instance running on another machine be served
// by the exact same dispatcher: its bytes arrive over the node link and are
// handed to an IpcSession through a pipe implementation that knows nothing
// about TCP accept().
//
// The interface is byte-level on purpose. A frame-level interface would have to
// own the read loop, which would mean the read completion handler owned the
// IpcSession while the IpcSession owned the pipe -- a reference cycle, and a
// second copy of the header/body state machine. Byte-level keeps one read loop
// (in IpcSession) and one place that knows how to move bytes.
//
// Phase 0 ships the socket implementation (SocketFramePipe) only. Phase 1 adds
// one per tunneled instance stream over the node link.

// Completion handler for a ReadSome/WriteSome. Cancellation and EOF arrive as
// an error_code, exactly like the asio operations underneath.
using FramePipeIoHandler = std::function<void(const asio::error_code&, std::size_t)>;

class FramePipe {
public:
    virtual ~FramePipe() = default;

    // Read exactly `len` bytes into `data`. The buffer stays owned by the
    // caller and must outlive the operation. Exactly one completion.
    virtual void ReadSome(void* data, std::size_t len, FramePipeIoHandler done) = 0;

    // Write exactly `data` (owned by the pipe from here on). Exactly one
    // completion. Implementations may not reorder writes; IpcSession already
    // serialises them.
    virtual void WriteSome(std::string data, FramePipeIoHandler done) = 0;

    // Tear the transport down. Pending operations complete with an error.
    virtual void Close() = 0;

    // Executor for session-owned timers (mission-end force exit, etc.).
    virtual asio::any_io_executor GetExecutor() = 0;

    // True when the peer is on this machine or a private network. The admin
    // path gates on it because the IPC protocol itself has no authentication.
    virtual bool IsLocalPeer() const = 0;

    // Peer identity for logs, e.g. "127.0.0.1:52344" or "node:eu-2".
    virtual std::string PeerLabel() const = 0;
};
