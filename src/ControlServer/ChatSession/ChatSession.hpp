#pragma once

#include <asio.hpp>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>

#include "src/ControlServer/Constants/TcpFunctions.h"
#include "src/ControlServer/Constants/TcpTypes.h"
#include "src/ControlServer/TcpSession/PacketView.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"

class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(asio::ip::tcp::socket socket);
    void start();
    void deliver(const std::vector<uint8_t>& msg);
    const std::string& get_player_name() const { return player_name_; }

private:
    asio::ip::tcp::socket socket_;
    std::vector<uint8_t> read_buf_;
    std::deque<std::vector<uint8_t>> write_queue_;
    std::string player_name_;

    template<typename... Bytes>
    void append(std::vector<uint8_t>& buffer, Bytes&&... bytes) {
        (buffer.push_back(static_cast<uint8_t>(bytes)), ...);
    }

    void WriteString(std::vector<uint8_t>& buffer, uint16_t type, const std::string& str) {
        append(buffer, type & 0xFF, type >> 8);
        uint16_t len = static_cast<uint16_t>(str.size());
        append(buffer, len & 0xFF, len >> 8);
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

    void Write4B(std::vector<uint8_t>& buffer, uint16_t type, uint32_t val) {
        append(buffer, type & 0xFF, type >> 8);
        append(buffer, val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF);
    }

    void do_read();
    void do_write();
    void handle_packet(const uint8_t* data, size_t length);
    void broadcast(const uint8_t* data, size_t length);
};
