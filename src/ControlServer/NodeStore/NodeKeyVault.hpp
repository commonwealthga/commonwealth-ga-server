#pragma once

#include <string>

// At-rest sealing for node pre-shared keys.
//
// A node PSK lets that node attach to the master and receive player bundles, so
// it is the one secret in the node tables. Keeping it plaintext in server.db
// would mean any copy of the database -- a backup, a dump pulled to a dev
// machine, the dashboard container -- is enough to impersonate a node.
//
// The sealing key is derived from `node_key_secret` in control-server.json, which
// never enters the database. Losing that secret means re-provisioning every node
// key; that is the intended cost of the property.
//
// This is confidentiality at rest, not a substitute for the link's own
// authentication: the handshake still proves possession of the PSK.
namespace NodeKeyVault {

// Install the secret. Anything not valid UTF-8/8+ bytes is accepted but logged;
// entropy is the caller's responsibility (openssl rand -hex 32).
void Configure(const std::string& secret);

// False when no secret is configured: callers must refuse to store or read keys
// rather than silently falling back to plaintext.
bool Configured();

// Seal `psk_hex` for storage. Empty result means the vault is not configured.
std::string Seal(const std::string& psk_hex);

// Recover a sealed blob. Returns false with `err` set on a bad tag / wrong
// secret / malformed blob.
bool Open(const std::string& sealed_hex, std::string& psk_hex, std::string& err);

}  // namespace NodeKeyVault
