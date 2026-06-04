#pragma once
#include <asio.hpp>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <cstdint>
#include "lib/nlohmann/json.hpp"

class IpcServer {
public:
    IpcServer(asio::io_context& io, uint16_t port);
    bool IsListening() const { return listening_; }

    // Pending ACK management (called from TcpSession on ASIO thread)
    static void RegisterPendingAck(const std::string& session_guid,
                                   std::function<void(bool success, int pawn_id)> callback);
    static void ClearPendingAck(const std::string& session_guid);

    // Send a JSON message to a specific game instance by instance_id.
    // Returns false if no session for that instance_id.
    static bool SendToInstance(int64_t instance_id, const std::string& json_msg);

    // Delivers ACK from dispatch -- looks up and invokes pending callback.
    // Called from IpcSession (same TU) when PLAYER_REGISTER_ACK arrives.
    static void DeliverAck(const std::string& session_guid, bool success, int pawn_id);

    // Successor spawn callback (set once at startup from main.cpp). Invoked
    // by the MSG_REQUEST_SUCCESSOR handler with the parent instance_id;
    // closure is responsible for reading parent's map/mode/queue from
    // InstanceRegistry, allocating a UDP port, calling InsertStarting with
    // predecessor_instance_id set, and spawning the game process. The
    // handler dedupes against existing DRAFTING/READY successors before
    // invoking the callback, so this only fires when an actual spawn is
    // needed.
    using SuccessorSpawner = std::function<void(int64_t parent_instance_id)>;
    static void SetSuccessorSpawner(SuccessorSpawner cb);

    // Invoked from the IPC dispatcher (IpcSession::dispatch in the same TU)
    // after the REQUEST_SUCCESSOR dedupe check decides we actually need a
    // fresh spawn. Internally checks whether a spawner has been registered
    // and logs either the invocation or the dropped request. Exists so
    // IpcSession doesn't need access to the private successor_spawner_.
    static void TriggerSuccessor(int64_t parent_instance_id);

    using AdminActionHandler = std::function<bool(const std::string& subtype,
                                                  const nlohmann::json& payload,
                                                  std::string& message)>;
    static void SetAdminToken(std::string token);
    static void SetAdminActionHandler(AdminActionHandler cb);

private:
    friend class IpcSession;

    void do_accept();

    static std::mutex ack_mutex_;
    static std::map<std::string, std::function<void(bool, int)>> pending_acks_;
    static SuccessorSpawner successor_spawner_;
    static std::string admin_token_;
    static AdminActionHandler admin_action_handler_;

    asio::io_context& io_;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    bool listening_ = false;
};
