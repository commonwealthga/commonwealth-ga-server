#pragma once

#include <asio.hpp>
#include <string>

// The website's write path into a player's account settings.
//
// Everything a player can toggle from chat (-togglebrokensuits, -toggleallsuits,
// -togglesolomode, -enabledlc/-disabledlc) lands in the game database, but the
// *live* effect for the suit toggles is an IPC player-action the control server
// pushes to the game DLL with the player's session GUID — and the DLL caches a
// player's preferences in memory, so writing the database alone would not be
// seen until they relogged. This endpoint therefore does both: persist, and
// push when the player is online.
//
// Deliberately a separate, small, write-only listener rather than part of the
// spectator overlay:
//   * it fails closed — no token configured means the listener refuses to
//     start (the overlay's "empty token = open" convention is fine for a
//     read-only endpoint and wrong for one that changes accounts);
//   * it defaults to loopback, because the only client is the website on the
//     same host;
//   * port 0 disables it entirely, so an older config file keeps working.
//
// Routes (both POST, JSON body, X-Admin-Token header required):
//   /player/preferences  {"user_id":2,"key":"show_broken_suits","value":"1"}
//   /player/dlc          {"user_id":2,"identifier":"carbon-capture","installed":true}
class PlayerSettingsHttpServer {
public:
    PlayerSettingsHttpServer(asio::io_context& io, const std::string& bind_address,
                             uint16_t port, std::string token);
    bool IsListening() const { return listening_; }

private:
    void do_accept();

    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
    std::string token_;
    bool listening_ = false;
    asio::steady_timer retry_timer_;
    int accept_errors_ = 0;
};
