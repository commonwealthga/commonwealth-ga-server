#pragma once

#include <asio.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>

// InstanceRelay -- one game instance's IPC connection on the node.
//
// The node does NOT interpret instance IPC. It relays each framed message to the
// master byte for byte and writes back whatever the master sends, which is why a
// remote instance is served by the master's existing dispatcher unchanged.
//
// The only message it parses is the FIRST one (INSTANCE_HELLO), to learn which
// instance the connection belongs to -- the DLL tells us, and the node has no
// other way to map a socket to an instance id.
//
// Framing here is the game DLL's IPC framing ([u32 len][json]), which is also
// what the master's IpcSession expects on the far side of the tunnel. The bytes
// are passed through verbatim, prefix included.
class InstanceRelay : public std::enable_shared_from_this<InstanceRelay> {
public:
    // Matches NodeLink::kMaxFrameSize so a relayed message can always be sealed
    // into one tunnel frame.
    static constexpr std::size_t kMaxFrameSize = 1u << 20;

    // Hand an encoded frame to the master. False means the link could not take it
    // (offline buffer full, or the send failed and closed the link).
    using SendToMaster = std::function<bool(std::int64_t instance_id, std::string encoded_frame)>;
    // Fired when INSTANCE_HELLO identifies which instance this connection is.
    // Fires BEFORE the HELLO is forwarded, so the caller is registered by the
    // time the master can possibly answer.
    using OnAdopted = std::function<void(std::int64_t instance_id,
                                        std::shared_ptr<InstanceRelay> relay)>;
    // Fired when the connection ends, so the node can stop tracking it.
    using OnReleased = std::function<void(std::int64_t instance_id)>;

    InstanceRelay(asio::ip::tcp::socket socket, SendToMaster send,
                  OnAdopted adopted, OnReleased released);

    void Start();

    // master -> instance. The frame is written verbatim.
    void Deliver(std::string encoded_frame);

    // Ceiling on frames held while waiting for INSTANCE_HELLO. The DLL's outbound
    // queue survives a reconnect, so a couple of messages can legitimately arrive
    // ahead of the fresh HELLO; more than this means something is wrong.
    static constexpr std::size_t kMaxPreHelloBytes = 256u << 10;

    // instance -> nothing. Idempotent.
    void Close(const std::string& reason);

    bool IsOpen() const { return open_; }
    std::int64_t InstanceId() const { return instance_id_; }
    const std::string& PeerLabel() const { return peer_label_; }

private:
    void do_read_header();
    void do_read_body(std::uint32_t len);
    void do_write();
    void write_to_instance(std::string encoded_frame);

    // Handling before the instance id is known. The DLL opens every connection
    // with PING and then INSTANCE_HELLO, and its outbound queue is not cleared on
    // a failed attempt, so a stray message can precede the HELLO too.
    void handle_pre_hello(std::uint32_t len);
    bool adopt(const std::string& body);   // learns the instance id from INSTANCE_HELLO
    void answer_ping();
    bool forward(std::uint32_t len, const std::string& body);
    void fail(const std::string& reason);

    asio::ip::tcp::socket socket_;
    std::string peer_label_;
    std::array<std::uint8_t, 4> header_{};
    std::string body_;
    std::deque<std::string> write_queue_;
    bool write_in_progress_ = false;
    bool open_ = false;
    bool started_ = false;
    std::int64_t instance_id_ = 0;
    // Encoded frames received before INSTANCE_HELLO identified the instance.
    // Sent to the master right after the HELLO, in their original order.
    std::string pre_hello_;

    SendToMaster send_;
    OnAdopted    adopted_;
    OnReleased   released_;
};
