#pragma once
#include <asio.hpp>
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Logger.hpp"

class ChatListener {
public:
    ChatListener(asio::io_context& io, uint16_t port)
        : acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
        Logger::Log("chat", "[ChatListener] Listening on port %d\n", port);
        do_accept();
    }
private:
    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket sock) {
            if (!ec) {
                Logger::Log("chat", "[ChatListener] New connection from %s\n",
                    sock.remote_endpoint().address().to_string().c_str());
                std::make_shared<ChatSession>(std::move(sock))->start();
            }
            do_accept();
        });
    }
    asio::ip::tcp::acceptor acceptor_;
};
