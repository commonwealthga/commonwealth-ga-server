#pragma once

#include <cstddef>
#include <cstdint>

// FrameIo.hpp -- the 4-byte length prefix shared by every node-link frame.
//
// Two frame shapes travel this wire, both length-prefixed:
//   handshake (before a session exists): [u32 len][json payload]
//   sealed    (after):                   [u32 len][mac 16][cipher]
// The receiver always knows which phase it is in, so the prefix alone is
// unambiguous.
//
// The length is attacker-controlled until a frame authenticates, so every read
// site must cap it (NodeLink::kMaxFrameSize) before allocating.

namespace NodeLinkIo {

inline void EncodeLength(std::uint32_t n, std::uint8_t out[4]) {
    out[0] = static_cast<std::uint8_t>(n & 0xFF);
    out[1] = static_cast<std::uint8_t>((n >> 8) & 0xFF);
    out[2] = static_cast<std::uint8_t>((n >> 16) & 0xFF);
    out[3] = static_cast<std::uint8_t>((n >> 24) & 0xFF);
}

inline std::uint32_t DecodeLength(const std::uint8_t in[4]) {
    return static_cast<std::uint32_t>(in[0])
         | (static_cast<std::uint32_t>(in[1]) << 8)
         | (static_cast<std::uint32_t>(in[2]) << 16)
         | (static_cast<std::uint32_t>(in[3]) << 24);
}

}  // namespace NodeLinkIo
