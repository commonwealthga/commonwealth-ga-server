// test_node_crypto.cpp -- unit tests for src/ControlServer/NodeLink/NodeCrypto.
//
// Run: make -C tests/node_crypto run
//
// The known-answer tests pin the vendored crypto (Monocypher) to published
// vectors; the handshake and frame tests pin the protocol glue we wrote on top
// of it, which is the part not covered by anyone else's review.

#include "src/ControlServer/NodeLink/NodeCrypto.hpp"

#include "monocypher.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

int g_failures = 0;
int g_checks   = 0;

void Check(bool cond, const char* what) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::printf("  FAIL  %s\n", what);
    }
}

void Section(const char* name) {
    std::printf("== %s\n", name);
}

std::string Hex(const std::uint8_t* p, std::size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (std::size_t i = 0; i < n; ++i) {
        s.push_back(d[p[i] >> 4]);
        s.push_back(d[p[i] & 0xF]);
    }
    return s;
}

NodeCrypto::Key KeyFromHexOrDie(const std::string& hex) {
    NodeCrypto::Key k{};
    if (!NodeCrypto::KeyFromHex(hex, k)) {
        std::printf("  FATAL bad key hex in test data\n");
        std::exit(1);
    }
    return k;
}

// ---------------------------------------------------------------------------

// RFC 7693 Appendix A: BLAKE2b-512("abc").
void TestBlake2bKat() {
    Section("BLAKE2b-512(\"abc\") known answer (RFC 7693 appendix A)");
    std::uint8_t hash[64];
    const std::uint8_t msg[] = {'a', 'b', 'c'};
    crypto_blake2b(hash, sizeof(hash), msg, sizeof(msg));
    const std::string got = Hex(hash, sizeof(hash));
    const std::string want =
        "ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d1"
        "7d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab92386edd4009923";
    if (got != want) std::printf("  got  %s\n  want %s\n", got.c_str(), want.c_str());
    Check(got == want, "blake2b-512(abc) matches RFC 7693");
}

// RFC 7748 section 6.1 X25519 test vector.
void TestX25519Kat() {
    Section("X25519 key agreement known answer (RFC 7748 6.1)");
    NodeCrypto::Key alice_sk = KeyFromHexOrDie(
        "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
    NodeCrypto::Key bob_sk = KeyFromHexOrDie(
        "5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb");
    const std::string alice_pk_want =
        "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a";
    const std::string bob_pk_want =
        "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f";
    const std::string shared_want =
        "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742";

    NodeCrypto::Key alice_pk{}, bob_pk{};
    crypto_x25519_public_key(alice_pk.data(), alice_sk.data());
    crypto_x25519_public_key(bob_pk.data(), bob_sk.data());
    Check(Hex(alice_pk.data(), 32) == alice_pk_want, "alice public key");
    Check(Hex(bob_pk.data(), 32) == bob_pk_want, "bob public key");

    std::uint8_t s1[32], s2[32];
    crypto_x25519(s1, alice_sk.data(), bob_pk.data());
    crypto_x25519(s2, bob_sk.data(), alice_pk.data());
    Check(Hex(s1, 32) == shared_want, "shared secret matches RFC 7748");
    Check(std::memcmp(s1, s2, 32) == 0, "shared secret is symmetric");
}

void TestRandom() {
    Section("RandomBytes");
    NodeCrypto::Key a{}, b{};
    Check(NodeCrypto::RandomBytes(a.data(), a.size()), "RandomBytes succeeds");
    Check(NodeCrypto::RandomBytes(b.data(), b.size()), "RandomBytes succeeds twice");
    Check(a != b, "two draws differ");
    std::uint8_t zero[32] = {0};
    Check(std::memcmp(a.data(), zero, 32) != 0, "draw is not all zeroes");
}

void TestKeyHex() {
    Section("Key hex round-trip");
    NodeCrypto::Key k{};
    Check(NodeCrypto::RandomBytes(k.data(), k.size()), "seed");
    const std::string hex = NodeCrypto::KeyToHex(k);
    Check(hex.size() == 64, "hex is 64 chars");
    NodeCrypto::Key back{};
    Check(NodeCrypto::KeyFromHex(hex, back), "KeyFromHex accepts our output");
    Check(back == k, "round-trips");

    Check(!NodeCrypto::KeyFromHex("", back), "rejects empty");
    Check(!NodeCrypto::KeyFromHex(std::string(63, 'a'), back), "rejects short");
    Check(!NodeCrypto::KeyFromHex(std::string(65, 'a'), back), "rejects long");
    Check(!NodeCrypto::KeyFromHex(std::string(64, 'z'), back), "rejects non-hex");
}

// Full handshake; returns true and fills both sessions on success.
bool Handshake(const NodeCrypto::Key& node_psk, const NodeCrypto::Key& master_psk,
               NodeCrypto::Session& node_session, NodeCrypto::Session& master_session,
               std::string* err = nullptr) {
    NodeCrypto::Initiator node("eu-north", node_psk);
    const std::string hello = node.Start();
    if (hello.empty()) return false;

    NodeCrypto::Responder master;
    std::string claimed_id, e;
    if (!master.OnHello(hello, claimed_id, e)) {
        if (err) *err = "OnHello: " + e;
        return false;
    }
    if (claimed_id != "eu-north") {
        if (err) *err = "wrong node_id";
        return false;
    }

    const std::string challenge = master.BuildChallenge(master_psk, e);
    if (challenge.empty()) {
        if (err) *err = "BuildChallenge: " + e;
        return false;
    }

    const std::string auth = node.OnChallenge(challenge, e);
    if (auth.empty()) {
        if (err) *err = "OnChallenge: " + e;
        return false;
    }

    std::string auth_ok;
    if (!master.OnAuth(auth, auth_ok, master_session, e)) {
        if (err) *err = "OnAuth: " + e;
        return false;
    }

    if (!node.OnAuthOk(auth_ok, node_session, e)) {
        if (err) *err = "OnAuthOk: " + e;
        return false;
    }
    return true;
}

void TestHandshakeOk() {
    Section("handshake completes and both sides agree");
    NodeCrypto::Key psk{};
    Check(NodeCrypto::RandomBytes(psk.data(), psk.size()), "seed psk");

    NodeCrypto::Session node_s, master_s;
    std::string err;
    Check(Handshake(psk, psk, node_s, master_s, &err), "handshake succeeds");
    if (!err.empty()) std::printf("  (%s)\n", err.c_str());
    Check(node_s.IsValid(), "node session valid");
    Check(master_s.IsValid(), "master session valid");

    // node -> master
    const std::string sealed = node_s.Seal(1, 4242, "hello master");
    Check(!sealed.empty(), "node seals a frame");
    std::uint8_t kind = 0;
    std::uint64_t stream = 0;
    std::string payload, oerr;
    Check(master_s.Open(sealed, kind, stream, payload, oerr), "master opens it");
    Check(kind == 1, "kind preserved");
    Check(stream == 4242, "stream preserved");
    Check(payload == "hello master", "payload preserved");

    // master -> node (opposite key direction)
    const std::string sealed2 = master_s.Seal(0, 0, "{\"ok\":true}");
    Check(!sealed2.empty(), "master seals a frame");
    Check(node_s.Open(sealed2, kind, stream, payload, oerr), "node opens it");
    Check(kind == 0 && stream == 0 && payload == "{\"ok\":true}", "reply round-trips");

    // Empty payload is legal (control ping).
    const std::string sealed3 = node_s.Seal(1, 7, "");
    Check(master_s.Open(sealed3, kind, stream, payload, oerr), "empty payload opens");
    Check(payload.empty() && stream == 7, "empty payload preserved");
}

void TestHandshakeWrongKey() {
    Section("handshake fails on a wrong node key");
    NodeCrypto::Key right{}, wrong{};
    NodeCrypto::RandomBytes(right.data(), right.size());
    NodeCrypto::RandomBytes(wrong.data(), wrong.size());

    NodeCrypto::Session a, b;
    std::string err;
    Check(!Handshake(right, wrong, a, b, &err), "handshake rejected");
    Check(!a.IsValid() && !b.IsValid(), "no session on either side");
    Check(err.find("OnAuth") == 0 || err.find("node failed") != std::string::npos,
          "rejection came from the proof check");
    std::printf("  rejection: %s\n", err.c_str());
}

// Tamper with one handshake field and expect the handshake to fail.
void TamperCase(const char* what, const std::string& tamper) {
    NodeCrypto::Key psk{};
    NodeCrypto::RandomBytes(psk.data(), psk.size());

    NodeCrypto::Initiator node("eu-north", psk);
    std::string hello = node.Start();
    if (tamper == "node_id") {
        const auto pos = hello.find("eu-north");
        if (pos != std::string::npos) hello.replace(pos, 8, "eu-south");
    }
    if (tamper == "hello_eph") {
        const auto pos = hello.find("\"eph\":\"");
        if (pos != std::string::npos) {
            // Flip one hex digit of the ephemeral key.
            const std::size_t at = pos + 7;
            hello[at] = (hello[at] == 'a') ? 'b' : 'a';
        }
    }

    NodeCrypto::Responder master;
    std::string claimed, err;
    if (!master.OnHello(hello, claimed, err)) {
        Check(true, what);  // rejected at parse time, also fine
        return;
    }
    std::string challenge = master.BuildChallenge(psk, err);
    if (challenge.empty()) { Check(true, what); return; }

    std::string auth = node.OnChallenge(challenge, err);
    if (auth.empty()) { Check(true, what); return; }

    std::string auth_ok;
    NodeCrypto::Session master_session;
    const bool ok = master.OnAuth(auth, auth_ok, master_session, err);
    Check(!ok, what);
    std::printf("  %-22s -> %s\n", what, ok ? "ACCEPTED (bug)" : err.c_str());
}

void TestTampering() {
    Section("transcript binding: tampered fields are rejected");
    TamperCase("tampered node_id", "node_id");
    TamperCase("tampered ephemeral key", "hello_eph");
}

void TestFrameRejections() {
    Section("frame rejections");
    NodeCrypto::Key psk{};
    NodeCrypto::RandomBytes(psk.data(), psk.size());
    NodeCrypto::Session node_s, master_s;
    Check(Handshake(psk, psk, node_s, master_s), "handshake");

    std::uint8_t kind = 0;
    std::uint64_t stream = 0;
    std::string payload, err;

    // --- a frame that opens, then the same frame replayed ---
    const std::string f0 = node_s.Seal(1, 1, "first");
    Check(!f0.empty(), "sealed f0");
    Check(master_s.Open(f0, kind, stream, payload, err), "f0 opens");
    Check(!master_s.Open(f0, kind, stream, payload, err), "replay rejected");
    std::printf("  replay           -> %s\n", err.c_str());

    // --- tampered tag ---
    std::string flipped = node_s.Seal(1, 1, "second");
    Check(!flipped.empty(), "sealed flipped");
    flipped[flipped.size() - 1] ^= 0x01;
    Check(!master_s.Open(flipped, kind, stream, payload, err), "tampered tag rejected");
    std::printf("  tampered tag     -> %s\n", err.c_str());

    // --- and now the session is finished, by design ---
    // The sender advanced its counter for both rejected frames, so the next
    // genuine frame must NOT open. A silent resync here would be a replay hole.
    const std::string after = node_s.Seal(1, 2, "after");
    Check(!master_s.Open(after, kind, stream, payload, err),
          "failed frame desyncs the session (no silent resync)");
    std::printf("  after failure    -> %s\n", err.c_str());

    // --- malformed sealed bodies, on a healthy session ---
    // These are hand-built rather than sealed, so they never consume the sender's
    // counter: body rejections are harmless, auth rejections are not. The [u32
    // len] prefix is the transport's, validated by NodeLink::Link (covered by
    // tests/node_link), so it does not appear here.
    NodeCrypto::Session node_f, master_f;
    Check(Handshake(psk, psk, node_f, master_f), "second handshake");
    Check(!master_f.Open(std::string(), kind, stream, payload, err),
          "empty body rejected");
    Check(!master_f.Open("xx", kind, stream, payload, err), "2-byte body rejected");

    std::string four(4, '\0');
    Check(!master_f.Open(four, kind, stream, payload, err), "4-byte body rejected");

    std::string under_min(NodeCrypto::kMacSize + 8, '\0');   // one byte short of mac+header
    Check(!master_f.Open(under_min, kind, stream, payload, err),
          "body one byte under the minimum rejected");

    // The session still works after body-only rejections.
    const std::string genuine = node_f.Seal(1, 5, "genuine");
    Check(master_f.Open(genuine, kind, stream, payload, err),
          "body rejections do not desync");
    Check(payload == "genuine" && stream == 5, "payload intact");

    // --- a frame from a different session under the same PSK ---
    // Keys are ephemeral per connection, so it must not open.
    const std::string other = node_s.Seal(1, 1, "from another session");
    Check(!master_f.Open(other, kind, stream, payload, err),
          "frame from a different session rejected");
}

void TestInitiatorRefusesForgedMaster() {
    Section("the node refuses a master that cannot prove the key");
    NodeCrypto::Key psk{};
    NodeCrypto::RandomBytes(psk.data(), psk.size());

    NodeCrypto::Initiator node("eu-north", psk);
    const std::string hello = node.Start();
    Check(!hello.empty(), "hello");

    // A fake master that answers with a well-formed but wrong AUTH_OK.
    NodeCrypto::Responder honest;
    std::string claimed, err;
    Check(honest.OnHello(hello, claimed, err), "parse hello");
    const std::string challenge = honest.BuildChallenge(psk, err);
    const std::string auth = node.OnChallenge(challenge, err);
    Check(!auth.empty(), "node produces auth");

    std::string forged = "{\"type\":\"NODE_AUTH_OK\",\"proof\":\"";
    forged += std::string(64, '0');
    forged += "\"}";
    NodeCrypto::Session s;
    Check(!node.OnAuthOk(forged, s, err), "forged AUTH_OK rejected");
    std::printf("  forged AUTH_OK   -> %s\n", err.c_str());
    Check(!node.Ok(), "initiator marked failed");
}

void TestGarbageHandshake() {
    Section("malformed handshake messages are rejected, not crashed on");
    NodeCrypto::Key psk{};
    NodeCrypto::RandomBytes(psk.data(), psk.size());

    NodeCrypto::Responder r;
    std::string id, err;
    Check(!r.OnHello("", id, err), "empty hello rejected");
    Check(!r.OnHello("not json", id, err), "non-JSON rejected");
    Check(!r.OnHello("[]", id, err), "array rejected");
    Check(!r.OnHello("{\"type\":\"NODE_HELLO\"}", id, err), "missing fields rejected");
    Check(!r.OnHello("{\"type\":\"NODE_HELLO\",\"node_id\":\"x\",\"proto\":\"9\","
                     "\"eph\":\"" + std::string(64, 'a') + "\",\"nonce\":\"" +
                     std::string(48, 'a') + "\",\"ts\":1}", id, err),
          "protocol version mismatch rejected");
    std::printf("  version mismatch -> %s\n", err.c_str());

    // Oversized handshake payload.
    const std::string huge(9000, 'x');
    Check(!r.OnHello(huge, id, err), "oversized rejected");

    NodeCrypto::Initiator n("n", psk);
    NodeCrypto::Session s;
    Check(n.OnChallenge("garbage", err).empty(), "garbage challenge rejected");
    Check(!n.OnAuthOk("garbage", s, err), "garbage auth_ok rejected");
}

}  // namespace

int main() {
    TestBlake2bKat();
    TestX25519Kat();
    TestRandom();
    TestKeyHex();
    TestHandshakeOk();
    TestHandshakeWrongKey();
    TestTampering();
    TestFrameRejections();
    TestInitiatorRefusesForgedMaster();
    TestGarbageHandshake();

    std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
