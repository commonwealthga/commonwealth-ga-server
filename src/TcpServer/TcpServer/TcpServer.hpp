#pragma once

#include "src/pch.hpp"
#include "src/TcpServer/TcpSession/TcpSession.hpp"

class TcpServer {
public:
    TcpServer(asio::io_context& io_context, short tcp_port)
        : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), tcp_port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket socket) {
                if (!ec) {
                    std::make_shared<TcpSession>(std::move(socket))->start();
                }
                do_accept();
            });
    }

	asio::ip::tcp::acceptor acceptor_;
};

