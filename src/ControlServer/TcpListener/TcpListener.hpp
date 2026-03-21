#pragma once
#include <asio.hpp>
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Logger.hpp"

class TcpListener {
public:
    TcpListener(asio::io_context& io, uint16_t port)
        : io_(io), acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
        Logger::Log("tcp", "[TcpListener] Listening on port %d\n", port);
        do_accept();
    }
private:
    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket sock) {
            if (!ec) {
                Logger::Log("tcp", "[TcpListener] New connection from %s\n",
                    sock.remote_endpoint().address().to_string().c_str());
                std::make_shared<TcpSession>(std::move(sock), io_)->start();
            }
            do_accept();
        });
    }
    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
};
