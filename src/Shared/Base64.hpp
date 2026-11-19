#pragma once

#include <cstdint>
#include <string>

// Base64.hpp -- standard (RFC 4648, with padding) base64.
//
// Used to carry content-snapshot bytes inside JSON control frames. Deliberately
// not a general-purpose dependency: binary artifacts go through this once per
// node update, so the 33% size overhead costs a few hundred KB in total and buys
// debug-friendliness (message dumps stay readable, no second frame kind, no new
// routing).
//
// ARCH: header-only, standard C++ only -- reachable from both halves if needed.

namespace Base64 {

inline std::string Encode(const std::uint8_t* data, std::size_t len) {
    static const char* kAlphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    std::size_t i = 0;
    for (; i + 2 < len; i += 3) {
        const std::uint32_t v = (static_cast<std::uint32_t>(data[i]) << 16)
                              | (static_cast<std::uint32_t>(data[i + 1]) << 8)
                              | static_cast<std::uint32_t>(data[i + 2]);
        out.push_back(kAlphabet[(v >> 18) & 0x3F]);
        out.push_back(kAlphabet[(v >> 12) & 0x3F]);
        out.push_back(kAlphabet[(v >> 6) & 0x3F]);
        out.push_back(kAlphabet[v & 0x3F]);
    }
    if (i < len) {
        std::uint32_t v = static_cast<std::uint32_t>(data[i]) << 16;
        const bool two = (i + 1 < len);
        if (two) v |= static_cast<std::uint32_t>(data[i + 1]) << 8;
        out.push_back(kAlphabet[(v >> 18) & 0x3F]);
        out.push_back(kAlphabet[(v >> 12) & 0x3F]);
        out.push_back(two ? kAlphabet[(v >> 6) & 0x3F] : '=');
        out.push_back('=');
    }
    return out;
}

// Returns false on a malformed input (bad length, bad character, bad padding).
// Strict on purpose: this decodes data that will become a database file.
inline bool Decode(const std::string& in, std::string& out) {
    auto value = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };

    if (in.size() % 4 != 0) return false;

    out.clear();
    out.reserve((in.size() / 4) * 3);

    for (std::size_t i = 0; i < in.size(); i += 4) {
        int quad[4];
        int padding = 0;
        for (int k = 0; k < 4; ++k) {
            const char c = in[i + k];
            if (c == '=') {
                // Padding may only appear in the last two positions of the group.
                if (k < 2) return false;
                quad[k] = 0;
                ++padding;
            } else {
                if (padding != 0) return false;   // data after padding
                quad[k] = value(c);
                if (quad[k] < 0) return false;
            }
        }
        const std::uint32_t v = (static_cast<std::uint32_t>(quad[0]) << 18)
                              | (static_cast<std::uint32_t>(quad[1]) << 12)
                              | (static_cast<std::uint32_t>(quad[2]) << 6)
                              | static_cast<std::uint32_t>(quad[3]);
        out.push_back(static_cast<char>((v >> 16) & 0xFF));
        if (padding < 2) out.push_back(static_cast<char>((v >> 8) & 0xFF));
        if (padding < 1) out.push_back(static_cast<char>(v & 0xFF));
    }
    return true;
}

}  // namespace Base64
