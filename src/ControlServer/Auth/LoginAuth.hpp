#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Server-side reconstruction of the client login credential check.
//
// The retail client never sends the plaintext password. On login it:
//   1. asks the server for a SESSION_GUID nonce (the challenge),
//   2. derives an RC4 key = MD5(MD5(utf16le(lower(user)+password+STATIC_SALT))),
//   3. sends BIN_BLOB = RC4(key, utf16le(guidString + lower(user))).
//
// RC4 key recovery is infeasible, so the server can't learn the password. But
// the client re-derives the key from scratch each login (CryptDuplicateKey
// resets RC4 to position 0), so the keystream is identical across logins for a
// given password. The server knows both the guid string (it issued the nonce)
// and the username, so it can recover that keystream:
//
//   keystream = BIN_BLOB XOR utf16le(guidString + lower(user))
//   verifier  = SHA256(keystream)
//
// Storing SHA256(keystream) gives a stable per-account verifier: register it on
// first login, recompute + compare on later logins. See
// .planning/2026-06-08-login-password-flow.md for the full RE.
namespace LoginAuth {

// Reproduce Windows UuidToStringW() output for the 16 nonce bytes the server
// issued. `issuedGuidHex` is the 32-char lowercase-hex string produced by
// TcpSession::GenerateSessionGuid() (byte order b0..b15 on the wire).
// Returns the canonical "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" form (lowercase,
// hyphenated, no braces) — note Data1/Data2/Data3 are little-endian. Returns ""
// if the input isn't exactly 32 hex characters.
std::string GuidStringFromIssuedHex(const std::string& issuedGuidHex);

// Compute the account verifier (SHA-256 of the recovered RC4 keystream) from a
// login attempt. `username` is the account name as received (ASCII; lowercased
// internally to match the client). Returns a 32-byte vector, or an empty vector
// if the inputs can't form a valid check (bad guid hex, empty blob, or a blob
// whose length doesn't match the expected plaintext length).
std::vector<uint8_t> ComputeVerifier(const std::string& issuedGuidHex,
                                     const std::string& username,
                                     const std::vector<uint8_t>& blob);

// SHA-256 of an arbitrary byte buffer (32-byte digest). Exposed for reuse/tests.
std::vector<uint8_t> Sha256(const uint8_t* data, size_t len);

} // namespace LoginAuth
