#include "src/ControlServer/IpcServer/SocketFramePipe.hpp"

namespace {

// Kept from IpcServer.cpp (its only user was the ADMIN_ACTION source gate).
bool IsPrivateOrLocalAddress(const asio::ip::address& address) {
    if (address.is_loopback()) return true;
    if (address.is_v4()) {
        const auto bytes = address.to_v4().to_bytes();
        const uint8_t a = bytes[0];
        const uint8_t b = bytes[1];
        if (a == 10) return true;
        if (a == 172 && b >= 16 && b <= 31) return true;
        if (a == 192 && b == 168) return true;
        if (a == 169 && b == 254) return true;
        return false;
    }
    if (address.is_v6()) {
        const auto bytes = address.to_v6().to_bytes();
        if ((bytes[0] & 0xfe) == 0xfc) return true;  // fc00::/7
        if (bytes[0] == 0xfe && (bytes[1] & 0xc0) == 0x80) return true;  // fe80::/10
    }
    return false;
}

}  // namespace

SocketFramePipe::SocketFramePipe(asio::ip::tcp::socket socket)
    : socket_(std::move(socket)) {}

void SocketFramePipe::ReadSome(void* data, std::size_t len, FramePipeIoHandler done) {
    // self keeps the socket alive while the operation is in flight; `done`
    // keeps the caller's session alive the same way the old inline lambdas did.
    auto self = shared_from_this();
    asio::async_read(
        socket_,
        asio::buffer(data, len),
        asio::transfer_exactly(len),
        [self, done = std::move(done)](const asio::error_code& ec, std::size_t n) {
            if (done) done(ec, n);
        });
}

void SocketFramePipe::WriteSome(std::string data, FramePipeIoHandler done) {
    auto self = shared_from_this();
    auto buf = std::make_shared<std::string>(std::move(data));
    asio::async_write(
        socket_,
        asio::buffer(*buf),
        [self, buf, done = std::move(done)](const asio::error_code& ec, std::size_t n) {
            if (done) done(ec, n);
        });
}

void SocketFramePipe::Close() {
    asio::error_code ec;
    socket_.close(ec);
}

asio::any_io_executor SocketFramePipe::GetExecutor() {
    return socket_.get_executor();
}

bool SocketFramePipe::IsLocalPeer() const {
    asio::error_code ec;
    auto remote = socket_.remote_endpoint(ec);
    if (ec) return false;
    return IsPrivateOrLocalAddress(remote.address());
}

std::string SocketFramePipe::PeerLabel() const {
    asio::error_code ec;
    auto remote = socket_.remote_endpoint(ec);
    if (ec) return "unknown";
    return remote.address().to_string() + ":" + std::to_string(remote.port());
}
