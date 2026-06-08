#pragma once

#include "src/pch.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include <deque>
#include <string>
#include <cstdint>
#include <array>
#include <vector>

// IpcClient -- DLL-side IPC transport.
// Connects to the control server over loopback TCP.
// Messages sent from the game thread are enqueued to a CRITICAL_SECTION-protected queue
// and sent by the ASIO background thread without blocking the UE3 tick.
// Automatically reconnects with exponential backoff on disconnect.

class IpcClient {
public:
    // Init -- stores host/port/instance_id, spawns IpcThread. Call once from ModuleThread.
    static void Init(const std::string& host, uint16_t port, int64_t instance_id);

    // Send -- game-thread API. Enqueues a JSON message for async send on the ASIO thread.
    static void Send(const std::string& json_msg);

    // Send-side helper for MSG_REQUEST_SUCCESSOR. Fire from the pre-warm
    // trigger (e.g. when the game-mode signals "round is about to end" —
    // mid-overtime, score-threshold-crossed, final-objective-50%-captured,
    // whatever's right for that mode). Control server dedupes against
    // existing DRAFTING/READY successors of this instance, so spamming is
    // safe. Idempotent.
    static void SendRequestSuccessor();

    // Send-side helper for MSG_REQUEST_REBALANCE. Fire once per setup phase,
    // ~5s before the setup timer ends. Control server resolves the queue
    // policy and dispatches any team moves; safe to call for any mode (control
    // server no-ops for pinned queues).
    static void SendRequestRebalance();

    // Report debug/chat command execution back to the control-server console.
    static void SendChatCommandAudit(const std::string& session_guid,
                                     const std::string& command,
                                     const std::string& outcome,
                                     const std::string& details = "");

    // DrainInbound -- game-thread API. Called from Actor__Tick each tick.
    // Processes all queued inbound messages received since the last call.
    static void DrainInbound();

    // GetInstanceId -- returns the instance_id assigned at Init().
    static int64_t GetInstanceId() { return instance_id_; }

    // IpcThread -- background thread entry point (CreateThread callback).
    static DWORD WINAPI IpcThread(LPVOID);

private:
    // Outbound queue -- game thread enqueues, ASIO thread dequeues.
    static CRITICAL_SECTION outbound_cs_;
    static std::deque<std::string> outbound_queue_;

    // Inbound queue -- ASIO thread enqueues, game thread dequeues (DrainInbound).
    static CRITICAL_SECTION inbound_cs_;
    static std::deque<std::string> inbound_queue_;

    static std::string host_;
    static uint16_t port_;
    static int64_t instance_id_;

    // Set by IpcThread on connect, cleared on disconnect.
    // Accessed from both game thread (Send post) and ASIO thread.
    // io_ctx_ writes are protected by the reconnect loop sequence
    // (ASIO thread sets, game thread only reads for posting).
    static asio::io_context* io_ctx_;

    // write_in_progress_ -- accessed only on the ASIO thread (no lock needed).
    static bool write_in_progress_;

    // socket_ -- accessed only on the ASIO thread.
    static asio::ip::tcp::socket* socket_;

    // Read buffers -- accessed only on ASIO thread.
    static std::array<uint8_t, 4> header_buf_;

    // ASIO-thread internal methods.
    static void do_read_header();
    static void do_read_body(uint32_t len);
    static void do_write();
    static void kick_write();
};
