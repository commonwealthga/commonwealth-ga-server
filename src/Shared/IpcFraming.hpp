#pragma once

#include <string>
#include <cstdint>
#include <cstring>
#include <vector>

// IpcFraming.hpp -- 4-byte little-endian length-prefix framing for the IPC wire protocol.
// ARCH: This file must compile on both i686 (DLL, Windows/Wine) and x86_64 (control server, Linux).
//       Include ONLY standard C++ headers. Never include pch.hpp, windows.h, or SDK headers.
//
// Wire format: [ uint32_t payload_len (LE, 4 bytes) ][ payload bytes ]
// Both sides are x86 (little-endian), matching the existing TcpSession framing convention.

namespace IpcFraming {

// Encode a payload string into a framed message.
// Returns a string of the form: [4-byte LE length][payload]
inline std::string Encode(const std::string& payload) {
    uint32_t len = static_cast<uint32_t>(payload.size());

    // Write 4-byte little-endian length prefix.
    char prefix[4];
    prefix[0] = static_cast<char>( len        & 0xFF);
    prefix[1] = static_cast<char>((len >>  8) & 0xFF);
    prefix[2] = static_cast<char>((len >> 16) & 0xFF);
    prefix[3] = static_cast<char>((len >> 24) & 0xFF);

    std::string frame;
    frame.reserve(4 + payload.size());
    frame.append(prefix, 4);
    frame.append(payload);
    return frame;
}

// Decode a framed message from a raw buffer.
//
// buf     -- pointer to the beginning of the receive buffer
// buflen  -- total number of valid bytes in the buffer
// out_payload_len -- set to the payload length declared in the frame header
// out_payload     -- set to the extracted payload string on success
//
// Returns true if a complete frame is present and was decoded.
// Returns false if the buffer is too short (header incomplete or payload truncated).
// Caller is responsible for advancing the read cursor by (4 + out_payload_len) on success.
inline bool Decode(const uint8_t* buf, size_t buflen,
                   uint32_t& out_payload_len, std::string& out_payload) {
    // Need at least 4 bytes for the length header.
    if (buflen < 4) return false;

    // Read 4-byte little-endian uint32_t.
    uint32_t payload_len =
          (static_cast<uint32_t>(buf[0])      )
        | (static_cast<uint32_t>(buf[1]) <<  8)
        | (static_cast<uint32_t>(buf[2]) << 16)
        | (static_cast<uint32_t>(buf[3]) << 24);

    // Need 4-byte header + payload_len bytes.
    if (buflen < 4 + static_cast<size_t>(payload_len)) return false;

    out_payload_len = payload_len;
    out_payload.assign(reinterpret_cast<const char*>(buf + 4), payload_len);
    return true;
}

} // namespace IpcFraming
