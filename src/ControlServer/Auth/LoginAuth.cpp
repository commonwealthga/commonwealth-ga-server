#include "src/ControlServer/Auth/LoginAuth.hpp"

#include <cstring>

namespace LoginAuth {

namespace {

// ---------------------------------------------------------------------------
// SHA-256 (FIPS 180-4) — self-contained, public-domain-style implementation.
// ---------------------------------------------------------------------------
struct Sha256Ctx {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t  buf[64];
    size_t   buflen;
};

inline uint32_t Rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

const uint32_t kK[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

void Sha256Init(Sha256Ctx& c) {
    c.state[0] = 0x6a09e667; c.state[1] = 0xbb67ae85;
    c.state[2] = 0x3c6ef372; c.state[3] = 0xa54ff53a;
    c.state[4] = 0x510e527f; c.state[5] = 0x9b05688c;
    c.state[6] = 0x1f83d9ab; c.state[7] = 0x5be0cd19;
    c.bitlen = 0;
    c.buflen = 0;
}

void Sha256Block(Sha256Ctx& c, const uint8_t* p) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = (uint32_t(p[i * 4]) << 24) | (uint32_t(p[i * 4 + 1]) << 16) |
               (uint32_t(p[i * 4 + 2]) << 8) | uint32_t(p[i * 4 + 3]);
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = c.state[0], b = c.state[1], cc = c.state[2], d = c.state[3];
    uint32_t e = c.state[4], f = c.state[5], g = c.state[6], h = c.state[7];
    for (int i = 0; i < 64; ++i) {
        uint32_t S1 = Rotr(e, 6) ^ Rotr(e, 11) ^ Rotr(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + S1 + ch + kK[i] + w[i];
        uint32_t S0 = Rotr(a, 2) ^ Rotr(a, 13) ^ Rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & cc) ^ (b & cc);
        uint32_t t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = cc; cc = b; b = a; a = t1 + t2;
    }
    c.state[0] += a; c.state[1] += b; c.state[2] += cc; c.state[3] += d;
    c.state[4] += e; c.state[5] += f; c.state[6] += g; c.state[7] += h;
}

void Sha256Update(Sha256Ctx& c, const uint8_t* data, size_t len) {
    c.bitlen += uint64_t(len) * 8;
    while (len > 0) {
        size_t take = 64 - c.buflen;
        if (take > len) take = len;
        std::memcpy(c.buf + c.buflen, data, take);
        c.buflen += take;
        data += take;
        len  -= take;
        if (c.buflen == 64) {
            Sha256Block(c, c.buf);
            c.buflen = 0;
        }
    }
}

void Sha256Final(Sha256Ctx& c, uint8_t out[32]) {
    uint64_t bitlen = c.bitlen;
    uint8_t pad = 0x80;
    Sha256Update(c, &pad, 1);
    uint8_t zero = 0x00;
    while (c.buflen != 56) Sha256Update(c, &zero, 1);
    uint8_t lenbe[8];
    for (int i = 0; i < 8; ++i) lenbe[i] = uint8_t(bitlen >> (56 - i * 8));
    Sha256Update(c, lenbe, 8);
    for (int i = 0; i < 8; ++i) {
        out[i * 4]     = uint8_t(c.state[i] >> 24);
        out[i * 4 + 1] = uint8_t(c.state[i] >> 16);
        out[i * 4 + 2] = uint8_t(c.state[i] >> 8);
        out[i * 4 + 3] = uint8_t(c.state[i]);
    }
}

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------
inline char LowerAscii(char ch) {
    return (ch >= 'A' && ch <= 'Z') ? char(ch - 'A' + 'a') : ch;
}

inline int HexNibble(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

// Parse the 32-char hex into 16 bytes. Returns false on malformed input.
bool ParseGuidHex(const std::string& hex, uint8_t out[16]) {
    if (hex.size() != 32) return false;
    for (int i = 0; i < 16; ++i) {
        int hi = HexNibble(hex[i * 2]);
        int lo = HexNibble(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = uint8_t((hi << 4) | lo);
    }
    return true;
}

inline void AppendHexByte(std::string& s, uint8_t b) {
    static const char* d = "0123456789abcdef";
    s.push_back(d[b >> 4]);
    s.push_back(d[b & 0x0F]);
}

} // namespace

std::vector<uint8_t> Sha256(const uint8_t* data, size_t len) {
    Sha256Ctx c;
    Sha256Init(c);
    Sha256Update(c, data, len);
    std::vector<uint8_t> out(32);
    Sha256Final(c, out.data());
    return out;
}

std::string GuidStringFromIssuedHex(const std::string& issuedGuidHex) {
    uint8_t b[16];
    if (!ParseGuidHex(issuedGuidHex, b)) return std::string();

    // UuidToStringW formats the 16 wire bytes as a Windows GUID:
    //   Data1 (b0..b3) and Data2 (b4..b5), Data3 (b6..b7) are little-endian;
    //   Data4 (b8..b15) is printed in order. Result is lowercase, hyphenated.
    std::string s;
    s.reserve(36);
    AppendHexByte(s, b[3]); AppendHexByte(s, b[2]); AppendHexByte(s, b[1]); AppendHexByte(s, b[0]);
    s.push_back('-');
    AppendHexByte(s, b[5]); AppendHexByte(s, b[4]);
    s.push_back('-');
    AppendHexByte(s, b[7]); AppendHexByte(s, b[6]);
    s.push_back('-');
    AppendHexByte(s, b[8]); AppendHexByte(s, b[9]);
    s.push_back('-');
    for (int i = 10; i < 16; ++i) AppendHexByte(s, b[i]);
    return s;
}

std::vector<uint8_t> ComputeVerifier(const std::string& issuedGuidHex,
                                     const std::string& username,
                                     const std::vector<uint8_t>& blob) {
    if (blob.empty()) return std::vector<uint8_t>();

    const std::string guidStr = GuidStringFromIssuedHex(issuedGuidHex);
    if (guidStr.empty()) return std::vector<uint8_t>();

    // Expected plaintext = utf16le( guidString + lower(username) ), no NUL.
    std::string plain = guidStr;
    plain.reserve(guidStr.size() + username.size());
    for (char ch : username) plain.push_back(LowerAscii(ch));

    const size_t expectedLen = plain.size() * 2;
    if (blob.size() != expectedLen) return std::vector<uint8_t>();

    // keystream = blob XOR utf16le(plain)
    std::vector<uint8_t> keystream(expectedLen);
    for (size_t i = 0; i < plain.size(); ++i) {
        const uint8_t lo = uint8_t(plain[i]);   // ASCII low byte
        const uint8_t hi = 0x00;                // UTF-16LE high byte
        keystream[i * 2]     = blob[i * 2]     ^ lo;
        keystream[i * 2 + 1] = blob[i * 2 + 1] ^ hi;
    }

    return Sha256(keystream.data(), keystream.size());
}

} // namespace LoginAuth
