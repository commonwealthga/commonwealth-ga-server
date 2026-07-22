#pragma once

#include <asio.hpp>
#include <string>

// Minimal read-only HTTP/1.1 GET server, hand-rolled with raw asio (no
// dependency worth pulling in for one JSON endpoint). Serves live spectator
// overlay state from SpectatorOverlayState:
//
//   GET /overlay?instance_id=<int64>&token=<string>
//
// Token is checked against the configured overlay_token (empty = open
// access, same convention as admin_token). Runs on the same io_context as
// every other listener in this process — no locking needed on that account,
// SpectatorOverlayState has its own mutex regardless since it's also written
// from the IPC dispatch path on the same io_context.
class OverlayHttpServer {
public:
    OverlayHttpServer(asio::io_context& io, uint16_t port, std::string token);
    bool IsListening() const { return listening_; }

private:
    void do_accept();

    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
    std::string token_;
    bool listening_ = false;
};
