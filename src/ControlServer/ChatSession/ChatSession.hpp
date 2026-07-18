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

    // Deliver a system chat line to one player's chat session, if they have
    // one. Used by TcpSession (e.g. friends "player not found" feedback).
    static void SystemMessageTo(const std::string& player_name, const std::string& text);

private:
    asio::ip::tcp::socket socket_;
    asio::steady_timer bind_retry_timer_;
    // Scratch buffer for one async_read_some call. TCP is a byte stream, so a
    // single read can return a partial packet, exactly one packet, or several
    // packets concatenated — appended into `accumulator_` and drained from
    // there in do_read(). On loopback small payloads happen to arrive 1:1, so
    // the unframed code worked locally but broke between remote clients.
    std::vector<uint8_t> read_buf_;
    std::vector<uint8_t> accumulator_;
    std::deque<std::vector<uint8_t>> write_queue_;
    std::string player_name_;
    std::string session_guid_;
    int bind_retry_attempts_ = 0;
    bool announced_join_ = false;
    // Single-shot lifecycle guard. Set by TearDown(); checked in every async
    // callback so a session that's already been torn down (e.g. by a write
    // error) is inert if a stale callback fires later. Without this the read
    // and write error paths could double-remove from g_chat_sessions or
    // double-broadcast the leave announcement.
    bool closed_ = false;

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
    bool BindPlayerFromRegistry(const char* reason);
    void ScheduleBindRetry();
    void AnnounceJoinIfNeeded();

    // Single tear-down path. Idempotent via `closed_`. On entry, optionally
    // broadcasts a "<player> has left the chat" system message if the session
    // was handshaken, then clears the write queue, closes the socket, and
    // removes self from g_chat_sessions so further broadcasts skip it.
    void TearDown(const char* reason);
};
