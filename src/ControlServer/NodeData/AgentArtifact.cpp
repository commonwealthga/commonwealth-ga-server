#include "src/ControlServer/NodeData/AgentArtifact.hpp"

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeData/FileHash.hpp"

#include <cstdio>
#include <filesystem>
#include <sys/stat.h>
#include <system_error>
#include <vector>

namespace {

constexpr const char* kTag = "node";

AgentArtifact::Info g_agent;
AgentArtifact::Info g_dll;

// What the DLL file looked like when we last hashed it. stat() is the change
// signal: hashing 9 MB on every tick would be wasteful, and mtime+size is what a
// rebuild always changes.
std::int64_t g_dll_mtime = 0;
std::int64_t g_dll_size  = 0;

bool StatFile(const std::string& path, std::int64_t& mtime, std::int64_t& size) {
    if (path.empty()) return false;
#ifdef _WIN32
    struct _stat st {};
    if (_stat(path.c_str(), &st) != 0) return false;
#else
    struct stat st {};
    if (stat(path.c_str(), &st) != 0) return false;
#endif
    mtime = static_cast<std::int64_t>(st.st_mtime);
    size  = static_cast<std::int64_t>(st.st_size);
    return true;
}

AgentArtifact::Info Describe(const std::string& path, const char* what) {
    AgentArtifact::Info info;
    if (path.empty()) return info;

    info.hash = FileHash::OfFile(path);
    if (info.hash.empty()) {
        Logger::Log(kTag,
            "[Artifact] cannot hash the %s at '%s' -- it will not be offered to nodes\n",
            what, path.c_str());
        return info;
    }
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return info;
    std::fseek(f, 0, SEEK_END);
    info.size = static_cast<std::int64_t>(std::ftell(f));
    std::fclose(f);
    if (info.size <= 0) return info;

    info.path = path;
    info.ok   = true;
    Logger::Log(kTag, "[Artifact] %s '%s' %lld bytes hash=%s\n", what, path.c_str(),
        (long long)info.size, info.hash.c_str());
    return info;
}

}  // namespace

namespace AgentArtifact {

void Init(const ControlServerConfig& cfg) {
    if (!cfg.node_auto_update) {
        // Detection only. The master still knows what it is (hash printed below),
        // but offers nothing, so no node ever starts a transfer.
        g_agent = Describe(FileHash::SelfPath(), "agent (not offered)");
        g_dll   = Info{};
        g_agent.ok = false;
        Logger::Log(kTag,
            "[Artifact] node_auto_update is off -- nodes are not offered builds\n");
        return;
    }

    // The agent artifact is whatever this process is running from. Resolving it
    // through /proc/self/exe means the answer is right even when the binary was
    // renamed over since it started (the mapped inode keeps the old bytes, which
    // is what we are running).
    g_agent = Describe(FileHash::SelfPath(), "agent");

    // The proxy DLL is built separately (it is the Windows-side half), so the
    // master only offers one when the operator points at the build artifact.
    //
    // Resolved to an absolute path once, here: ReadChunk() re-opens the file for
    // every chunk, so a relative path would depend on the working directory
    // staying put for the whole of a multi-chunk transfer. Absolute from the
    // start also makes the startup log name the file unambiguously.
    std::string dll_path = cfg.node_dll;
    if (!dll_path.empty() && std::filesystem::path(dll_path).is_relative()) {
        std::error_code ec;
        const auto abs = std::filesystem::absolute(dll_path, ec);
        if (!ec) dll_path = abs.string();
    }
    g_dll = Describe(dll_path, "proxy DLL");
    if (g_dll.ok) StatFile(g_dll.path, g_dll_mtime, g_dll_size);
    if (cfg.node_dll.empty()) {
        Logger::Log(kTag,
            "[Artifact] node_dll is not set -- nodes will not be offered a proxy DLL\n");
    }
}

bool Refresh() {
    if (!g_dll.ok) return false;

    std::int64_t mtime = 0, size = 0;
    if (!StatFile(g_dll.path, mtime, size)) {
        // The file went away. Keep offering the last good bytes rather than
        // yanking the offer: a node mid-transfer should finish, and a missing
        // file is an operator error we have already logged once.
        return false;
    }
    if (mtime == g_dll_mtime && size == g_dll_size) return false;   // nothing to do
    g_dll_mtime = mtime;
    g_dll_size  = size;

    const std::string hash = FileHash::OfFile(g_dll.path);
    if (hash.empty()) {
        Logger::Log(kTag, "[Artifact] proxy DLL became unreadable at '%s'\n",
            g_dll.path.c_str());
        return false;
    }
    if (hash == g_dll.hash && size == g_dll.size) {
        Logger::Log(kTag,
            "[Artifact] proxy DLL touched but unchanged (%s) -- offer stands\n",
            hash.c_str());
        return false;
    }

    Logger::Log(kTag,
        "[Artifact] proxy DLL changed: %s (%lld bytes) -> %s (%lld bytes); "
        "re-announcing to connected nodes\n",
        g_dll.hash.c_str(), (long long)g_dll.size, hash.c_str(), (long long)size);
    g_dll.hash = hash;
    g_dll.size = size;
    return true;
}

const Info& Agent() { return g_agent; }
const Info& Dll() { return g_dll; }

bool ReadChunk(bool dll, std::int64_t offset, std::int64_t len, std::string& out,
               std::string& err) {
    const Info& info = dll ? g_dll : g_agent;
    if (!info.ok) {
        err = dll ? "no proxy DLL artifact configured" : "no agent artifact";
        return false;
    }
    if (offset < 0 || len <= 0 || offset >= info.size) {
        err = "range out of bounds";
        return false;
    }
    if (offset + len > info.size) len = info.size - offset;

    std::FILE* f = std::fopen(info.path.c_str(), "rb");
    if (!f) {
        err = "cannot open the artifact file";
        return false;
    }
    if (std::fseek(f, static_cast<long>(offset), SEEK_SET) != 0) {
        std::fclose(f);
        err = "seek failed";
        return false;
    }
    std::vector<char> buf(static_cast<std::size_t>(len));
    const std::size_t got = std::fread(buf.data(), 1, buf.size(), f);
    std::fclose(f);
    if (got == 0) {
        err = "short read";
        return false;
    }
    out.assign(buf.data(), got);
    return true;
}

}  // namespace AgentArtifact
