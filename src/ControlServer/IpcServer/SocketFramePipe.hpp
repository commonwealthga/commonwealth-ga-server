#pragma once

#include "src/ControlServer/IpcServer/FramePipe.hpp"
#include <asio.hpp>
#include <memory>

// SocketFramePipe.hpp -- FramePipe over a TCP socket.
//
// Used for game instances spawned on this machine (they dial the IPC port over
// loopback) and for admin clients.

class SocketFramePipe : public FramePipe,
                        public std::enable_shared_from_this<SocketFramePipe> {
public:
    explicit SocketFramePipe(asio::ip::tcp::socket socket);

    void ReadSome(void* data, std::size_t len, FramePipeIoHandler done) override;
    void WriteSome(std::string data, FramePipeIoHandler done) override;
    void Close() override;
    asio::any_io_executor GetExecutor() override;
    bool IsLocalPeer() const override;
    std::string PeerLabel() const override;

private:
    asio::ip::tcp::socket socket_;
};
