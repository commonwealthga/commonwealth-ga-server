#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

// NodeCrypto.hpp -- authentication, key agreement and frame encryption for the
// master <-> node link.
//
// Design: A1 in .planning/2026-11-18-multi-node-architecture.md. Each node has
// one independent 32-byte pre-shared key, provisioned at enrolment and stored on
// the master in nodes.json. A four-message handshake proves both sides hold that
// key and agrees an ephemeral X25519 secret, so a later key leak does not expose
// recorded traffic.
//
//   node   -> master  NODE_HELLO    { node_id, eph, nonce_n, ts }
//   master -> node    NODE_CHALLENGE{ eph, nonce_m, ts }
//   node   -> master  NODE_AUTH     { proof }
//   master -> node    NODE_AUTH_OK  { proof }
//
// Both proofs are BLAKE2b-256 keyed with the PSK over a length-prefixed
// transcript that binds node_id, both ephemeral public keys, both nonces and the
// master's timestamp. Binding node_id is what stops node B authenticating as
// node A. The two proofs use different labels so neither can be reflected.
//
// After the handshake every frame is XChaCha20-Poly1305 with a per-direction key
// and a nonce of (16-byte derived IV || 64-bit counter). The counter is the
// replay defence: frames must arrive in exact order, which a TCP link already
// guarantees, so a replayed or reordered frame is rejected rather than
// decrypted.
//
// Open() failing means the connection is finished: the sender advances its
// counter on every Seal(), so once the receiver rejects a frame the two sides
// cannot resync. Recovery is a fresh handshake, never a skip. (Framing errors
// that never reach the MAC -- empty buffer, bad length prefix -- do not consume
// the counter and are therefore harmless.)

namespace NodeCrypto {

constexpr std::size_t kKeySize   = 32;  // PSK, X25519 secret/public, session key
constexpr std::size_t kNonceSize = 24;  // XChaCha20 nonce
constexpr std::size_t kMacSize   = 16;  // Poly1305 tag

using Key   = std::array<std::uint8_t, kKeySize>;
using Nonce = std::array<std::uint8_t, kNonceSize>;

// Protocol version carried in NODE_HELLO. Bump when the transcript or the frame
// layout changes; a mismatch is refused at handshake time, not mid-stream.
constexpr const char* kProtocolVersion = "1";

// Fill `out` with `len` cryptographically secure random bytes. Returns false if
// the platform RNG is unavailable -- callers must abort the handshake rather
// than fall back to something predictable.
bool RandomBytes(void* out, std::size_t len);

// A PSK or session key as the lowercase hex the config files use.
std::string KeyToHex(const Key& key);
bool KeyFromHex(const std::string& hex, Key& out);

// ---------------------------------------------------------------------------
// Session -- frame codec for an authenticated link. Cheap to copy; one instance
// per connection per direction pair.
// ---------------------------------------------------------------------------

class Session {
public:
    Session() = default;

    // initiator = true on the node side (which owns k_c2s for sending).
    Session(const Key& k_c2s, const Key& k_s2c, bool initiator);

    bool IsValid() const { return valid_; }

    // Encrypt one frame. Returns the sealed body [mac 16][cipher], where the
    // plaintext is [kind u8][stream u64 LE][payload]. Returns "" if the session
    // is invalid or the counter is exhausted.
    //
    // The transport's [u32 len] prefix is NOT included: framing belongs to
    // whoever owns the byte stream (NodeLink::Link), not to the crypto layer.
    std::string Seal(std::uint8_t kind, std::uint64_t stream, const std::string& payload);

    // Decrypt one sealed body (as produced by Seal, length prefix already
    // stripped by the caller). Rejects a short body, a bad tag, and any frame
    // whose counter is not the next expected one (replay / reorder / truncation).
    bool Open(const std::string& sealed_body, std::uint8_t& kind, std::uint64_t& stream,
              std::string& payload, std::string& err);

private:
    Key     send_key_{};
    Key     recv_key_{};
    std::array<std::uint8_t, 16> send_iv_{};
    std::array<std::uint8_t, 16> recv_iv_{};
    std::uint64_t send_counter_ = 0;
    std::uint64_t recv_counter_ = 0;
    bool    valid_ = false;
};

// ---------------------------------------------------------------------------
// Initiator -- the node side of the handshake.
// ---------------------------------------------------------------------------

class Initiator {
public:
    Initiator(std::string node_id, const Key& psk);

    // Step 1: the NODE_HELLO payload.
    std::string Start();

    // Step 2: consume NODE_CHALLENGE, return the NODE_AUTH payload. Returns ""
    // and sets `err` if the challenge is malformed.
    std::string OnChallenge(const std::string& payload, std::string& err);

    // Step 3: consume NODE_AUTH_OK. On success fills `session` with the agreed
    // keys and verifies that the master really holds the PSK.
    bool OnAuthOk(const std::string& payload, Session& session, std::string& err);

    // False once the handshake failed; the caller should drop the connection.
    bool Ok() const { return ok_; }

private:
    std::string node_id_;
    Key         psk_{};
    Key         eph_priv_{};
    Key         eph_pub_{};
    Nonce       nonce_n_{};
    Key         eph_pub_m_{};
    Nonce       nonce_m_{};
    std::uint64_t ts_n_ = 0;   // ours, sent in NODE_HELLO
    std::uint64_t ts_m_ = 0;   // master's, from NODE_CHALLENGE
    bool        challenge_seen_ = false;
    bool        ok_ = false;
};

// ---------------------------------------------------------------------------
// Responder -- the master side of the handshake.
// ---------------------------------------------------------------------------

class Responder {
public:
    // Step 1: parse NODE_HELLO. Deliberately does NOT authenticate -- there is
    // no key yet -- it only extracts the claimed node_id and the peer's
    // handshake material. The caller looks the PSK up by node_id and must
    // refuse an unknown id before calling BuildChallenge.
    bool OnHello(const std::string& payload, std::string& node_id, std::string& err);

    // Step 2: produce NODE_CHALLENGE for the PSK of the node from OnHello.
    std::string BuildChallenge(const Key& psk, std::string& err);

    // Step 3: verify NODE_AUTH; on success fill `auth_ok_payload` (to send back)
    // and `session`. Rejects a wrong PSK, a tampered transcript, and a protocol
    // version mismatch.
    bool OnAuth(const std::string& payload, std::string& auth_ok_payload,
                Session& session, std::string& err);

private:
    std::string node_id_;
    std::string proto_;
    Key         psk_{};
    Key         eph_priv_{};
    Key         eph_pub_{};
    Nonce       nonce_m_{};
    Key         eph_pub_n_{};
    Nonce       nonce_n_{};
    std::uint64_t ts_n_ = 0;
    std::uint64_t ts_m_ = 0;
    bool        hello_seen_ = false;
};

}  // namespace NodeCrypto
