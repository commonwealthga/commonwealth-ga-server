#pragma once
#include <asio.hpp>
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Logger.hpp"

class ChatListener {
public:
    ChatListener(asio::io_context& io, uint16_t port)
        : acceptor_(io) {
        asio::error_code ec;
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            Logger::Log("chat", "[ChatListener] Open failed on port %d: %s\n", port, ec.message().c_str());
            return;
        }
        // Restore the implicit SO_REUSEADDR that asio's single-arg-endpoint
        // acceptor ctor used to set. Without it, restarting while a chat
        // client lingers in TIME_WAIT makes bind() fail with EADDRINUSE for
        // up to 60s.
        acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) {
            Logger::Log("chat", "[ChatListener] set_option(reuse_address) failed on port %d: %s\n", port, ec.message().c_str());
            acceptor_.close();
            return;
        }
        acceptor_.bind(endpoint, ec);
        if (ec) {
            Logger::Log("chat", "[ChatListener] Bind failed on port %d: %s\n", port, ec.message().c_str());
            acceptor_.close();
            return;
        }
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            Logger::Log("chat", "[ChatListener] Listen failed on port %d: %s\n", port, ec.message().c_str());
            acceptor_.close();
            return;
        }
        listening_ = true;
        Logger::Log("chat", "[ChatListener] Listening on port %d\n", port);
        do_accept();
    }
    bool IsListening() const { return listening_; }
private:
    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket sock) {
            if (!ec) {
                Logger::Log("chat", "[ChatListener] New connection from %s\n",
                    sock.remote_endpoint().address().to_string().c_str());
                std::make_shared<ChatSession>(std::move(sock))->start();
            }
            if (listening_) do_accept();
        });
    }
    asio::ip::tcp::acceptor acceptor_;
    bool listening_ = false;
};
