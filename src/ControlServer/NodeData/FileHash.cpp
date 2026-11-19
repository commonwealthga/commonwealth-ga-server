#include "src/ControlServer/NodeData/FileHash.hpp"

#include "monocypher.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace {

constexpr std::size_t kHashSize = 32;   // BLAKE2b-256
constexpr std::size_t kChunk     = 64 * 1024;

std::string Hex(const std::uint8_t* data, std::size_t len) {
    static const char* kDigits = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(kDigits[data[i] >> 4]);
        out.push_back(kDigits[data[i] & 0x0F]);
    }
    return out;
}

}  // namespace

namespace FileHash {

std::string OfBytes(const void* data, std::size_t len) {
    std::uint8_t digest[kHashSize];
    crypto_blake2b(digest, sizeof(digest),
                   static_cast<const std::uint8_t*>(data), len);
    return Hex(digest, sizeof(digest));
}

std::string OfFile(const std::string& path) {
    if (path.empty()) return "";
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return "";

    // Streaming, because these are multi-megabyte binaries and the agent may be
    // asked to hash itself while instances are running.
    crypto_blake2b_ctx ctx;
    crypto_blake2b_init(&ctx, kHashSize);

    std::vector<std::uint8_t> buf(kChunk);
    std::size_t total = 0;
    for (;;) {
        const std::size_t got = std::fread(buf.data(), 1, buf.size(), f);
        if (got > 0) {
            crypto_blake2b_update(&ctx, buf.data(), got);
            total += got;
        }
        if (got < buf.size()) break;   // eof or error
    }
    const bool read_error = std::ferror(f) != 0;
    std::fclose(f);
    if (read_error) return "";
    if (total == 0) return "";         // an empty artifact is never a valid one

    std::uint8_t digest[kHashSize];
    crypto_blake2b_final(&ctx, digest);
    return Hex(digest, sizeof(digest));
}

std::string SelfPath() {
#ifdef _WIN32
    char buf[MAX_PATH];
    const DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) return "";
    return std::string(buf, n);
#else
    char buf[PATH_MAX];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return "";
    return std::string(buf, static_cast<std::size_t>(n));
#endif
}

}  // namespace FileHash
