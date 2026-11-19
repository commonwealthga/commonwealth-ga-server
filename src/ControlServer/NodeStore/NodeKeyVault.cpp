#include "src/ControlServer/NodeStore/NodeKeyVault.hpp"

#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeLink/NodeCrypto.hpp"

#include "monocypher.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

using Key   = std::array<std::uint8_t, 32>;
using Nonce = std::array<std::uint8_t, 24>;

constexpr std::size_t kMacSize = 16;

bool        g_configured = false;
Key         g_key{};

std::string ToHex(const std::uint8_t* data, std::size_t len) {
    static const char* kHex = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(kHex[data[i] >> 4]);
        out.push_back(kHex[data[i] & 0x0F]);
    }
    return out;
}

bool FromHex(const std::string& hex, std::vector<std::uint8_t>& out) {
    if (hex.size() % 2 != 0) return false;
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    out.clear();
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int hi = nibble(hex[i]);
        const int lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }
    return true;
}

}  // namespace

namespace NodeKeyVault {

void Configure(const std::string& secret) {
    g_configured = false;
    g_key.fill(0);
    if (secret.empty()) return;

    // BLAKE2b-256 as the KDF. Fine because the secret is expected to be random
    // rather than a memorable passphrase -- a slow KDF would only matter for
    // low-entropy input, and this runs at startup on both master and node paths.
    crypto_blake2b(g_key.data(), g_key.size(),
                   reinterpret_cast<const std::uint8_t*>(secret.data()), secret.size());
    g_configured = true;
    if (secret.size() < 32) {
        Logger::Log("node",
            "[NodeKeyVault] node_key_secret is only %zu bytes; use a high-entropy "
            "secret (openssl rand -hex 32)\n", secret.size());
    }
}

bool Configured() { return g_configured; }

std::string Seal(const std::string& psk_hex) {
    if (!g_configured || psk_hex.empty()) return "";

    Nonce nonce{};
    if (!NodeCrypto::RandomBytes(nonce.data(), nonce.size())) return "";

    std::vector<std::uint8_t> cipher(psk_hex.size());
    std::array<std::uint8_t, kMacSize> mac{};
    crypto_aead_lock(cipher.data(), mac.data(), g_key.data(), nonce.data(), nullptr, 0,
                     reinterpret_cast<const std::uint8_t*>(psk_hex.data()), psk_hex.size());

    // nonce || mac || cipher: one opaque hex string in a TEXT column.
    std::string blob = ToHex(nonce.data(), nonce.size());
    blob += ToHex(mac.data(), mac.size());
    blob += ToHex(cipher.data(), cipher.size());
    return blob;
}

bool Open(const std::string& sealed_hex, std::string& psk_hex, std::string& err) {
    if (!g_configured) {
        err = "node_key_secret is not configured";
        return false;
    }
    std::vector<std::uint8_t> blob;
    if (!FromHex(sealed_hex, blob)) {
        err = "sealed key is not valid hex";
        return false;
    }
    const std::size_t header = sizeof(Nonce) + kMacSize;
    if (blob.size() <= header) {
        err = "sealed key is truncated";
        return false;
    }

    Nonce nonce{};
    std::memcpy(nonce.data(), blob.data(), nonce.size());
    const std::uint8_t* mac = blob.data() + nonce.size();
    const std::uint8_t* cipher = blob.data() + header;
    const std::size_t cipher_len = blob.size() - header;

    std::vector<std::uint8_t> plain(cipher_len);
    const int mismatch = crypto_aead_unlock(plain.data(), mac, g_key.data(), nonce.data(),
                                            nullptr, 0, cipher, cipher_len);
    if (mismatch != 0) {
        crypto_wipe(plain.data(), plain.size());
        err = "sealed key failed to open (wrong node_key_secret?)";
        return false;
    }
    psk_hex.assign(reinterpret_cast<const char*>(plain.data()), plain.size());
    crypto_wipe(plain.data(), plain.size());
    return true;
}

}  // namespace NodeKeyVault
