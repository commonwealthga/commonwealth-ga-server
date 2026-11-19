# Monocypher 4.0.3 (vendored)

Source: https://github.com/LoupVaillant/Monocypher tag `4.0.3`.

`monocypher.c` / `monocypher.h` are copied verbatim. Both match the tag's git
tree exactly (git blob SHA-1):

| file | blob |
|---|---|
| `monocypher.c` | `c7c5450e84e4dffda44514d9f918162f1830de56` |
| `monocypher.h` | `cf635e88e4f5851e1f58a9e2b3e85b8029a73861` |

Re-verify with `git hash-object lib/monocypher/monocypher.{c,h}`.

Used by `src/ControlServer/NodeLink/NodeCrypto.cpp` for the master↔node link:
X25519 (key agreement), XChaCha20-Poly1305 (`crypto_aead_lock`/`unlock`, 24-byte
nonce), BLAKE2b keyed mode (PSK proof + session key derivation), and
`crypto_verify32` (constant-time comparison).

Not used, so not vendored: `optional/monocypher-ed25519.c` (signatures),
Argon2, Elligator.

The file header says "Monocypher version __git__" because the tag string is
substituted at release-archive time; the released 4.0.3 tarball carries the same
bytes, so the version is recorded here instead.

Licence: dual-licensed BSD-2-Clause / CC0-1.0 — see `LICENCE.md`.
