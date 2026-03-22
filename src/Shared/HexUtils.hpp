#pragma once

// HexUtils.hpp -- Header-only hex encode/decode utility.
// ARCH: This file must compile on both i686 (DLL, Windows/Wine) and x86_64 (control server, Linux).
//       Include ONLY standard C++ headers. Never include pch.hpp, windows.h, or SDK headers.

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace HexUtils {

inline std::string hex_encode(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return oss.str();
}

inline std::string hex_encode(const std::vector<uint8_t>& data) {
    return hex_encode(data.data(), data.size());
}

inline std::vector<uint8_t> hex_decode(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
        bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    return bytes;
}

} // namespace HexUtils
