#include "src/ControlServer/NodeLink/NodeCrypto.hpp"
#include "src/Shared/HexUtils.hpp"
#include "lib/nlohmann/json.hpp"

#include "monocypher.h"

#include <cstring>
#include <ctime>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/random.h>
#include <unistd.h>
#endif

// NodeCrypto.cpp -- see NodeCrypto.hpp for the protocol. The one rule in here:
// every failure path must fail closed (return false / ""), never fall back to an
// unauthenticated or predictable state.

namespace NodeCrypto {
namespace {

// Domain-separation labels. Distinct for each use so a proof can never be
// mistaken for a key block, and c2s is never interchangeable with s2c.
constexpr const char* kLabelAuthNode   = "ga-node-auth-v1";
constexpr const char* kLabelAuthMaster = "ga-node-auth-v1-master";
constexpr const char* kLabelKdfC2S     = "ga-node-c2s";
constexpr const char* kLabelKdfS2C     = "ga-node-s2c";
constexpr const char* kLabelIv         = "ga-node-iv";

constexpr std::size_t kProofSize = 32;

int NibbleToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Strict fixed-size hex decode. Unlike HexUtils::hex_decode this rejects wrong
// lengths and non-hex characters instead of silently truncating.
bool DecodeHexFixed(const std::string& hex, std::uint8_t* out, std::size_t len) {
    if (hex.size() != len * 2) return false;
    for (std::size_t i = 0; i < len; ++i) {
        const int hi = NibbleToInt(hex[2 * i]);
        const int lo = NibbleToInt(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
    }
    return true;
}

void PutU32(std::vector<std::uint8_t>& v, std::uint32_t x) {
    for (int i = 0; i < 4; ++i) v.push_back(static_cast<std::uint8_t>(x >> (8 * i)));
}
void PutU64(std::vector<std::uint8_t>& v, std::uint64_t x) {
    for (int i = 0; i < 8; ++i) v.push_back(static_cast<std::uint8_t>(x >> (8 * i)));
}
// Length-prefixed so no two distinct strings can produce the same bytes.
void PutStr(std::vector<std::uint8_t>& v, const std::string& s) {
    PutU32(v, static_cast<std::uint32_t>(s.size()));
    v.insert(v.end(), s.begin(), s.end());
}
void PutBytes(std::vector<std::uint8_t>& v, const std::uint8_t* p, std::size_t n) {
    v.insert(v.end(), p, p + n);
}

// BLAKE2b-256 keyed with the PSK over the full handshake transcript. Binding
// node_id, both ephemeral keys and both nonces is what makes the proof
// specific to this node and this connection.
void ComputeProof(const Key& psk, const char* label,
                  const std::string& node_id,
                  const Key& pub_n, const Key& pub_m,
                  const Nonce& nonce_n, const Nonce& nonce_m,
                  std::uint64_t ts_n, std::uint64_t ts_m,
                  std::uint8_t out[kProofSize]) {
    std::vector<std::uint8_t> t;
    t.reserve(256);
    PutStr(t, label);
    PutStr(t, node_id);
    PutBytes(t, pub_n.data(), pub_n.size());
    PutBytes(t, pub_m.data(), pub_m.size());
    PutBytes(t, nonce_n.data(), nonce_n.size());
    PutBytes(t, nonce_m.data(), nonce_m.size());
    PutU64(t, ts_n);
    PutU64(t, ts_m);
    crypto_blake2b_keyed(out, kProofSize, psk.data(), psk.size(), t.data(), t.size());
    crypto_wipe(t.data(), t.size());
}

bool AllZero(const std::uint8_t* p, std::size_t n) {
    std::uint8_t acc = 0;
    for (std::size_t i = 0; i < n; ++i) acc |= p[i];
    return acc == 0;
}

// One direction's key + IV from the X25519 shared secret. The PSK is the
// BLAKE2b key, which is what keeps the derived keys node-specific even if two
// nodes somehow agreed the same ephemeral secret.
void DeriveDirection(const Key& psk, const char* label, const std::uint8_t shared[32],
                     const Nonce& nonce_n, const Nonce& nonce_m,
                     Key& out_key, std::array<std::uint8_t, 16>& out_iv) {
    std::vector<std::uint8_t> m;
    m.reserve(96);
    PutStr(m, label);
    PutBytes(m, shared, 32);
    PutBytes(m, nonce_n.data(), nonce_n.size());
    PutBytes(m, nonce_m.data(), nonce_m.size());

    std::uint8_t block[64];
    crypto_blake2b_keyed(block, sizeof(block), psk.data(), psk.size(), m.data(), m.size());
    std::memcpy(out_key.data(), block, 32);
    std::memcpy(out_iv.data(), block + 32, 16);
    crypto_wipe(block, sizeof(block));
    crypto_wipe(m.data(), m.size());
}

// Both sides run this with their own private key and the peer's public key; the
// X25519 shared secret is symmetric, so the c2s/s2c split comes from the labels.
bool BuildSession(const Key& psk, const Key& my_priv, const Key& peer_pub,
                  const Nonce& nonce_n, const Nonce& nonce_m,
                  bool initiator, Session& out) {
    std::uint8_t shared[32];
    crypto_x25519(shared, my_priv.data(), peer_pub.data());

    // All-zero output means a low-order peer public key: no usable secret.
    if (AllZero(shared, sizeof(shared))) {
        crypto_wipe(shared, sizeof(shared));
        return false;
    }

    Key k_c2s{}, k_s2c{};
    std::array<std::uint8_t, 16> iv_c2s{}, iv_s2c{};
    DeriveDirection(psk, kLabelKdfC2S, shared, nonce_n, nonce_m, k_c2s, iv_c2s);
    DeriveDirection(psk, kLabelKdfS2C, shared, nonce_n, nonce_m, k_s2c, iv_s2c);
    crypto_wipe(shared, sizeof(shared));

    out = Session(k_c2s, k_s2c, initiator);
    crypto_wipe(k_c2s.data(), k_c2s.size());
    crypto_wipe(k_s2c.data(), k_s2c.size());
    crypto_wipe(iv_c2s.data(), iv_c2s.size());
    crypto_wipe(iv_s2c.data(), iv_s2c.size());
    return true;
}

void BuildNonce(const std::array<std::uint8_t, 16>& iv, std::uint64_t counter,
                std::uint8_t nonce[24]) {
    std::memcpy(nonce, iv.data(), 16);
    for (int i = 0; i < 8; ++i) nonce[16 + i] = static_cast<std::uint8_t>(counter >> (8 * i));
}

bool ParseJson(const std::string& payload, nlohmann::json& out, std::string& err) {
    if (payload.size() > 8192) {
        err = "handshake message too large";
        return false;
    }
    out = nlohmann::json::parse(payload, nullptr, /*allow_exceptions=*/false);
    if (out.is_discarded() || !out.is_object()) {
        err = "handshake message is not a JSON object";
        return false;
    }
    return true;
}

bool GetHexField(const nlohmann::json& j, const char* key,
                 std::uint8_t* out, std::size_t len) {
    if (!j.contains(key) || !j[key].is_string()) return false;
    return DecodeHexFixed(j[key].get<std::string>(), out, len);
}

// nlohmann stores a plain non-negative integer literal as number_integer, not
// number_unsigned, so both have to be accepted here.
bool GetU64Field(const nlohmann::json& j, const char* key, std::uint64_t& out) {
    if (!j.contains(key)) return false;
    const auto& v = j[key];
    if (v.is_number_unsigned()) {
        out = v.get<std::uint64_t>();
        return true;
    }
    if (v.is_number_integer()) {
        const auto signed_v = v.get<std::int64_t>();
        if (signed_v < 0) return false;
        out = static_cast<std::uint64_t>(signed_v);
        return true;
    }
    return false;
}

}  // namespace

// ---------------------------------------------------------------------------
// RandomBytes
// ---------------------------------------------------------------------------

bool RandomBytes(void* out, std::size_t len) {
#ifdef _WIN32
    if (len == 0) return true;
    return BCryptGenRandom(nullptr, static_cast<PUCHAR>(out), static_cast<ULONG>(len),
                           BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
    auto* p = static_cast<std::uint8_t*>(out);
    std::size_t got = 0;
    while (got < len) {
        const ssize_t n = getrandom(p + got, len - got, 0);
        if (n > 0) {
            got += static_cast<std::size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        break;  // ENOSYS or a hard error: fall back to /dev/urandom
    }
    if (got == len) return true;

    // Kernels without getrandom(2).
    const int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return false;
    while (got < len) {
        const ssize_t n = read(fd, p + got, len - got);
        if (n > 0) {
            got += static_cast<std::size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        close(fd);
        return false;
    }
    close(fd);
    return true;
#endif
}

std::string KeyToHex(const Key& key) {
    return HexUtils::hex_encode(key.data(), key.size());
}

bool KeyFromHex(const std::string& hex, Key& out) {
    return DecodeHexFixed(hex, out.data(), out.size());
}

// ---------------------------------------------------------------------------
// Session
// ---------------------------------------------------------------------------

Session::Session(const Key& k_c2s, const Key& k_s2c, bool initiator) {
    // Per-direction IVs are derived from the direction key so both ends agree
    // without extra wire fields.
    auto derive_iv = [](const Key& key, std::array<std::uint8_t, 16>& iv) {
        std::uint8_t block[32];
        const std::uint8_t label[] = "ga-node-iv";
        crypto_blake2b_keyed(block, sizeof(block), key.data(), key.size(),
                             label, sizeof(label) - 1);
        std::memcpy(iv.data(), block, 16);
        crypto_wipe(block, sizeof(block));
    };

    send_key_ = initiator ? k_c2s : k_s2c;
    recv_key_ = initiator ? k_s2c : k_c2s;
    derive_iv(send_key_, send_iv_);
    derive_iv(recv_key_, recv_iv_);
    send_counter_ = 0;
    recv_counter_ = 0;
    valid_ = true;
}

std::string Session::Seal(std::uint8_t kind, std::uint64_t stream, const std::string& payload) {
    if (!valid_) return "";
    if (send_counter_ == UINT64_MAX) return "";  // exhausted: force a reconnect

    std::string plain;
    plain.reserve(9 + payload.size());
    plain.push_back(static_cast<char>(kind));
    for (int i = 0; i < 8; ++i) {
        plain.push_back(static_cast<char>((stream >> (8 * i)) & 0xFF));
    }
    plain += payload;

    std::uint8_t nonce[24];
    BuildNonce(send_iv_, send_counter_, nonce);

    // [mac 16][cipher]. The length prefix is the transport's business; it is
    // added by whatever is writing to a byte stream (NodeLink::Link).
    std::string out;
    out.resize(kMacSize + plain.size());

    auto* mac = reinterpret_cast<std::uint8_t*>(&out[0]);
    auto* cipher = reinterpret_cast<std::uint8_t*>(&out[kMacSize]);
    // No AAD: the monotonic per-direction nonce counter already binds the frame
    // to its position in the stream, and kind/stream ride inside the plaintext
    // under the MAC.
    crypto_aead_lock(cipher, mac, send_key_.data(), nonce, nullptr, 0,
                     reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());

    ++send_counter_;
    return out;
}

bool Session::Open(const std::string& sealed_body, std::uint8_t& kind,
                   std::uint64_t& stream, std::string& payload, std::string& err) {
    if (!valid_) {
        err = "session not established";
        return false;
    }
    // [mac 16][cipher]. The caller has already stripped and validated the
    // transport's length prefix.
    if (sealed_body.size() < kMacSize + 9) {
        err = "sealed frame too short";
        return false;
    }

    const std::size_t plain_len = sealed_body.size() - kMacSize;
    std::string plain;
    plain.resize(plain_len);

    const auto* b = reinterpret_cast<const std::uint8_t*>(sealed_body.data());

    std::uint8_t nonce[24];
    BuildNonce(recv_iv_, recv_counter_, nonce);

    const int mismatch = crypto_aead_unlock(
        reinterpret_cast<std::uint8_t*>(&plain[0]),
        b,
        recv_key_.data(), nonce, nullptr, 0,
        b + kMacSize, plain_len);
    if (mismatch != 0) {
        // Fatal, not recoverable. The counter is deliberately not consumed (a
        // forger should not be able to advance our state), but the *sender* has
        // already advanced its own counter, so the two sides can never agree
        // again on this session. The caller must drop the connection and
        // re-handshake; accepting a skip would open a replay window.
        err = "frame failed authentication";
        return false;
    }
    ++recv_counter_;

    const auto* p = reinterpret_cast<const std::uint8_t*>(plain.data());
    kind = p[0];
    stream = 0;
    for (int i = 0; i < 8; ++i) {
        stream |= static_cast<std::uint64_t>(p[1 + i]) << (8 * i);
    }
    payload.assign(plain.data() + 9, plain_len - 9);
    return true;
}

// ---------------------------------------------------------------------------
// Initiator (node side)
// ---------------------------------------------------------------------------

Initiator::Initiator(std::string node_id, const Key& psk)
    : node_id_(std::move(node_id)), psk_(psk) {
    ok_ = !node_id_.empty()
       && RandomBytes(eph_priv_.data(), eph_priv_.size())
       && RandomBytes(nonce_n_.data(), nonce_n_.size());
    if (ok_) {
        crypto_x25519_public_key(eph_pub_.data(), eph_priv_.data());
    }
}

std::string Initiator::Start() {
    if (!ok_) return "";

    ts_n_ = static_cast<std::uint64_t>(std::time(nullptr));

    nlohmann::json j;
    j["type"]    = "NODE_HELLO";
    j["proto"]   = kProtocolVersion;
    j["node_id"] = node_id_;
    j["eph"]     = HexUtils::hex_encode(eph_pub_.data(), eph_pub_.size());
    j["nonce"]   = HexUtils::hex_encode(nonce_n_.data(), nonce_n_.size());
    j["ts"]      = ts_n_;
    return j.dump();
}

std::string Initiator::OnChallenge(const std::string& payload, std::string& err) {
    if (!ok_) {
        err = "handshake already failed";
        return "";
    }
    nlohmann::json j;
    if (!ParseJson(payload, j, err)) return "";
    if (!j.contains("type") || !j["type"].is_string() ||
        j["type"].get<std::string>() != "NODE_CHALLENGE") {
        err = "expected NODE_CHALLENGE";
        return "";
    }
    if (!GetHexField(j, "eph", eph_pub_m_.data(), eph_pub_m_.size()) ||
        !GetHexField(j, "nonce", nonce_m_.data(), nonce_m_.size()) ||
        !GetU64Field(j, "ts", ts_m_)) {
        err = "malformed NODE_CHALLENGE";
        return "";
    }
    if (AllZero(eph_pub_m_.data(), eph_pub_m_.size())) {
        err = "NODE_CHALLENGE carried an empty ephemeral key";
        return "";
    }
    challenge_seen_ = true;

    std::uint8_t proof[kProofSize];
    ComputeProof(psk_, kLabelAuthNode, node_id_, eph_pub_, eph_pub_m_,
                 nonce_n_, nonce_m_, ts_n_, ts_m_, proof);

    nlohmann::json out;
    out["type"]  = "NODE_AUTH";
    out["proof"] = HexUtils::hex_encode(proof, sizeof(proof));
    crypto_wipe(proof, sizeof(proof));
    return out.dump();
}

bool Initiator::OnAuthOk(const std::string& payload, Session& session, std::string& err) {
    if (!ok_ || !challenge_seen_) {
        err = "NODE_AUTH_OK out of order";
        return false;
    }
    nlohmann::json j;
    if (!ParseJson(payload, j, err)) return false;
    if (!j.contains("type") || !j["type"].is_string() ||
        j["type"].get<std::string>() != "NODE_AUTH_OK") {
        err = "expected NODE_AUTH_OK";
        return false;
    }
    std::uint8_t got[kProofSize];
    if (!GetHexField(j, "proof", got, sizeof(got))) {
        err = "malformed NODE_AUTH_OK";
        return false;
    }

    std::uint8_t want[kProofSize];
    ComputeProof(psk_, kLabelAuthMaster, node_id_, eph_pub_, eph_pub_m_,
                 nonce_n_, nonce_m_, ts_n_, ts_m_, want);

    // Constant time; a zero return means equal.
    const bool match = crypto_verify32(got, want) == 0;
    crypto_wipe(want, sizeof(want));
    crypto_wipe(got, sizeof(got));
    if (!match) {
        ok_ = false;
        err = "master failed to prove possession of the node key";
        return false;
    }

    if (!BuildSession(psk_, eph_priv_, eph_pub_m_, nonce_n_, nonce_m_,
                      /*initiator=*/true, session)) {
        ok_ = false;
        err = "key agreement failed";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Responder (master side)
// ---------------------------------------------------------------------------

bool Responder::OnHello(const std::string& payload, std::string& node_id, std::string& err) {
    nlohmann::json j;
    if (!ParseJson(payload, j, err)) return false;
    if (!j.contains("type") || !j["type"].is_string() ||
        j["type"].get<std::string>() != "NODE_HELLO") {
        err = "expected NODE_HELLO";
        return false;
    }
    if (!j.contains("node_id") || !j["node_id"].is_string() ||
        !j.contains("proto") || !j["proto"].is_string()) {
        err = "malformed NODE_HELLO";
        return false;
    }
    proto_ = j["proto"].get<std::string>();
    if (proto_ != kProtocolVersion) {
        err = "protocol version mismatch (master speaks " + std::string(kProtocolVersion) + ")";
        return false;
    }
    if (!GetHexField(j, "eph", eph_pub_n_.data(), eph_pub_n_.size()) ||
        !GetHexField(j, "nonce", nonce_n_.data(), nonce_n_.size()) ||
        !GetU64Field(j, "ts", ts_n_)) {
        err = "malformed NODE_HELLO";
        return false;
    }
    if (AllZero(eph_pub_n_.data(), eph_pub_n_.size())) {
        err = "NODE_HELLO carried an empty ephemeral key";
        return false;
    }
    node_id_ = j["node_id"].get<std::string>();
    // Bounded so an unauthenticated peer cannot inflate the transcript.
    if (node_id_.empty() || node_id_.size() > 64) {
        err = "node_id must be 1..64 characters";
        return false;
    }
    hello_seen_ = true;

    node_id = node_id_;
    return true;
}

std::string Responder::BuildChallenge(const Key& psk, std::string& err) {
    if (!hello_seen_) {
        err = "BuildChallenge before OnHello";
        return "";
    }
    psk_ = psk;
    if (!RandomBytes(eph_priv_.data(), eph_priv_.size()) ||
        !RandomBytes(nonce_m_.data(), nonce_m_.size())) {
        err = "system RNG unavailable";
        return "";
    }
    crypto_x25519_public_key(eph_pub_.data(), eph_priv_.data());
    ts_m_ = static_cast<std::uint64_t>(std::time(nullptr));

    nlohmann::json j;
    j["type"]  = "NODE_CHALLENGE";
    j["eph"]   = HexUtils::hex_encode(eph_pub_.data(), eph_pub_.size());
    j["nonce"] = HexUtils::hex_encode(nonce_m_.data(), nonce_m_.size());
    j["ts"]    = ts_m_;
    return j.dump();
}

bool Responder::OnAuth(const std::string& payload, std::string& auth_ok_payload,
                       Session& session, std::string& err) {
    if (!hello_seen_ || ts_m_ == 0) {
        err = "OnAuth before BuildChallenge";
        return false;
    }
    nlohmann::json j;
    if (!ParseJson(payload, j, err)) return false;
    if (!j.contains("type") || !j["type"].is_string() ||
        j["type"].get<std::string>() != "NODE_AUTH") {
        err = "expected NODE_AUTH";
        return false;
    }
    std::uint8_t got[kProofSize];
    if (!GetHexField(j, "proof", got, sizeof(got))) {
        err = "malformed NODE_AUTH";
        return false;
    }

    std::uint8_t want[kProofSize];
    ComputeProof(psk_, kLabelAuthNode, node_id_, eph_pub_n_, eph_pub_,
                 nonce_n_, nonce_m_, ts_n_, ts_m_, want);

    const bool match = crypto_verify32(got, want) == 0;
    crypto_wipe(want, sizeof(want));
    crypto_wipe(got, sizeof(got));
    if (!match) {
        err = "node failed to prove possession of its key";
        return false;
    }

    if (!BuildSession(psk_, eph_priv_, eph_pub_n_, nonce_n_, nonce_m_,
                      /*initiator=*/false, session)) {
        err = "key agreement failed";
        return false;
    }

    std::uint8_t proof[kProofSize];
    ComputeProof(psk_, kLabelAuthMaster, node_id_, eph_pub_n_, eph_pub_,
                 nonce_n_, nonce_m_, ts_n_, ts_m_, proof);
    nlohmann::json out;
    out["type"]  = "NODE_AUTH_OK";
    out["proof"] = HexUtils::hex_encode(proof, sizeof(proof));
    crypto_wipe(proof, sizeof(proof));
    auth_ok_payload = out.dump();
    return true;
}

}  // namespace NodeCrypto
