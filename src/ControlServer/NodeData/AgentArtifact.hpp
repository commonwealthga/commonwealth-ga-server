#pragma once

#include <cstdint>
#include <string>

struct ControlServerConfig;

// The two artifacts a node may need that are not content: the node agent binary
// (the control server itself, in node mode) and the proxy DLL the game instances
// load.
//
// Both are hashed once at startup and served over the already-authenticated link,
// chunked, with the hash announced in NODE_WELCOME. The hash is what a node
// verifies before writing anything -- the link is the trust channel, so no
// detached signature is needed (design S11).
//
// The agent artifact is *this* executable: whatever the master is running is
// exactly what nodes should converge to, in either direction. That also means a
// rollback needs no special case -- run the older build and it offers the older
// bytes.
namespace AgentArtifact {

struct Info {
    bool        ok   = false;
    std::string path;
    std::string hash;     // BLAKE2b-256, lowercase hex
    std::int64_t size = 0;
};

// Hash both artifacts once. Safe to call before the link exists; re-callable.
void Init(const ControlServerConfig& cfg);

// Re-checks the proxy DLL on disk and re-hashes it if it changed.
//
// The whole point of a DLL-only deploy is that it needs no control-server
// restart (a restart kicks every player to the login screen), so the offered DLL
// cannot be frozen at startup. Called from the registry tick: a stat() when
// nothing changed, a 9 MB hash when the file was touched -- cheap enough to poll
// and it keeps "drop a new version.dll in and nodes pick it up" true.
//
// The agent artifact deliberately does NOT do this: it is the binary this process
// is running, and offering bytes we are not executing would be a lie.
// Returns true when the offered DLL changed, so the caller can re-announce it.
bool Refresh();

const Info& Agent();
const Info& Dll();

// Reads [offset, offset+len) of the requested artifact. `dll` selects the proxy
// DLL instead of the agent. Returns false with `err` set on any failure.
bool ReadChunk(bool dll, std::int64_t offset, std::int64_t len, std::string& out,
               std::string& err);

}  // namespace AgentArtifact
