#pragma once
#include <asio.hpp>
#include <memory>
#include <cstdint>

class IpcServer {
public:
    IpcServer(asio::io_context& io, uint16_t port);
private:
    void do_accept();
    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
};
