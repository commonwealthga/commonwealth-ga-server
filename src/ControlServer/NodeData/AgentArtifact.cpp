#include "src/ControlServer/NodeData/AgentArtifact.hpp"

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeData/FileHash.hpp"

#include <cstdio>
#include <vector>

namespace {

constexpr const char* kTag = "node";

AgentArtifact::Info g_agent;
AgentArtifact::Info g_dll;

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
    g_dll = Describe(cfg.node_dll, "proxy DLL");
    if (cfg.node_dll.empty()) {
        Logger::Log(kTag,
            "[Artifact] node_dll is not set -- nodes will not be offered a proxy DLL\n");
    }
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
