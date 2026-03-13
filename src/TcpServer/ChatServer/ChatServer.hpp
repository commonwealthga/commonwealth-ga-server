#pragma once

#include "src/pch.hpp"
#include "src/TcpServer/ChatSession/ChatSession.hpp"

class ChatServer {
public:
    ChatServer(asio::io_context& io, short port)
        : acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket socket) {
                if (!ec)
                    std::make_shared<ChatSession>(std::move(socket))->start();
                do_accept();
            });
    }

    asio::ip::tcp::acceptor acceptor_;
};
