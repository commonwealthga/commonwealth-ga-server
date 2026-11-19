#pragma once

#include <cstddef>
#include <string>

// BLAKE2b-256 over a file or a buffer, lowercase hex.
//
// Needed by the update path in two directions: the master hashes the artifacts it
// is about to serve (so a node can verify what it downloaded), and both sides
// hash the running binaries to know whether they already have that byte image.
//
// BLAKE2b-256 rather than SHA-256 because Monocypher is already vendored for the
// node link -- no second crypto dependency for a hash.
namespace FileHash {

// "" on any failure (missing file, unreadable, empty); callers treat "" as "no
// identity" rather than as a hash that happens to be empty.
std::string OfFile(const std::string& path);

std::string OfBytes(const void* data, std::size_t len);

// Path of the running executable: /proc/self/exe on Linux, GetModuleFileName on
// Windows. "" if it cannot be resolved. Symlinks are resolved, so two launches via
// different names hash the same file.
std::string SelfPath();

}  // namespace FileHash
