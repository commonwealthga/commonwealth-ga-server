#pragma once
#include <asio.hpp>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <cstdint>

class IpcServer {
public:
    IpcServer(asio::io_context& io, uint16_t port);

    // Pending ACK management (called from TcpSession on ASIO thread)
    static void RegisterPendingAck(const std::string& session_guid,
                                   std::function<void(bool success, int pawn_id)> callback);
    static void ClearPendingAck(const std::string& session_guid);

    // Send a JSON message to the connected game instance.
    // Returns false if no IpcSession is connected.
    static bool SendToInstance(const std::string& json_msg);

    // Delivers ACK from dispatch -- looks up and invokes pending callback.
    // Called from IpcSession (same TU) when PLAYER_REGISTER_ACK arrives.
    static void DeliverAck(const std::string& session_guid, bool success, int pawn_id);

private:
    void do_accept();

    static std::mutex ack_mutex_;
    static std::map<std::string, std::function<void(bool, int)>> pending_acks_;

    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
};
