#pragma once

#include <string>

// Helpers for the addresses that go into client packets.
//
// TcpSession::WriteIP packs an address by splitting the string on '.' and parsing
// each token as an integer, so a hostname in one of those fields does not degrade
// -- it throws std::stoi and takes the master down. Anything that reaches WriteIP
// must therefore be a resolved dotted quad, which is why node addresses are
// resolved before they are used.
namespace NetAddress {

// True for "a.b.c.d" with four fields, each 0..255, digits only.
inline bool IsIPv4Literal(const std::string& s) {
    int fields = 0;
    std::size_t i = 0;
    while (i < s.size()) {
        std::size_t digits = 0;
        int value = 0;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            value = value * 10 + (s[i] - '0');
            if (++digits > 3) return false;
            ++i;
        }
        if (digits == 0 || value > 255) return false;
        ++fields;
        if (i == s.size()) break;
        if (s[i] != '.') return false;
        ++i;
        if (i == s.size()) return false;   // trailing dot
    }
    return fields == 4;
}

}  // namespace NetAddress
