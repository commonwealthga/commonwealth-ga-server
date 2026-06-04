#pragma once
#include <asio.hpp>
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Logger.hpp"

class TcpListener {
public:
    TcpListener(asio::io_context& io, uint16_t port)
        : io_(io), acceptor_(io) {
        asio::error_code ec;
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            Logger::Log("tcp", "[TcpListener] Open failed on port %d: %s\n", port, ec.message().c_str());
            return;
        }
        // Restore the implicit SO_REUSEADDR that asio's single-arg-endpoint
        // acceptor ctor used to set. Without it, restarting while a client
        // socket lingers in TIME_WAIT makes bind() fail with EADDRINUSE for
        // up to 60s.
        acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) {
            Logger::Log("tcp", "[TcpListener] set_option(reuse_address) failed on port %d: %s\n", port, ec.message().c_str());
            acceptor_.close();
            return;
        }
        acceptor_.bind(endpoint, ec);
        if (ec) {
            Logger::Log("tcp", "[TcpListener] Bind failed on port %d: %s\n", port, ec.message().c_str());
            acceptor_.close();
            return;
        }
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            Logger::Log("tcp", "[TcpListener] Listen failed on port %d: %s\n", port, ec.message().c_str());
            acceptor_.close();
            return;
        }
        listening_ = true;
        Logger::Log("tcp", "[TcpListener] Listening on port %d\n", port);
        do_accept();
    }
    bool IsListening() const { return listening_; }
private:
    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket sock) {
            if (!ec) {
                Logger::Log("tcp", "[TcpListener] New connection from %s\n",
                    sock.remote_endpoint().address().to_string().c_str());
                std::make_shared<TcpSession>(std::move(sock), io_)->start();
            }
            if (listening_) do_accept();
        });
    }
    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
    bool listening_ = false;
};
