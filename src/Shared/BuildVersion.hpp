#pragma once

#include "src/Shared/BuildVersion.gen.hpp"

#include <string>

// Build identity of the control server / node agent.
//
// Master and node run the same binary, so both sides report the same string and
// the master can compare a node's report against its own `kDescribe`.
//
// The identity is the commit, not a hand-maintained number: two builds of the
// same tag can be different code. `Describe()` is the full
// `tag-commits-gHASH[-dirty]` string, `Commit()` the full hash.
namespace BuildVersion {

// constexpr so they can initialise other compile-time constants, e.g.
// NodeMsg::kAgentVersion.
constexpr const char* Describe() { return kDescribe; }
constexpr const char* Commit() { return kCommit; }
constexpr bool IsDirty() { return kDirty; }

// Ordering between two describe strings. Parsed as MAJOR.MINOR.PATCH-COMMITS,
// ignoring the hash and any "v" prefix on the tag.
//
// Nodes converge to the master's build in EITHER direction (a rollback is a
// deploy of an older build, and nodes must follow it), so this does not gate an
// update. It exists to name the direction in logs and in the dashboard:
// "downgrading 0.1.60-2-gabc -> 0.1.58-1-gdef" should be visible, not inferred.
enum class Order {
    Unknown,   // either side is not parseable (e.g. "unknown", a hand-made tag)
    Older,
    Same,      // same tag and commit count -- may still be different code
    Newer,
};

struct Parsed {
    int  major   = 0;
    int  minor   = 0;
    int  patch   = 0;
    int  commits = 0;
    bool ok      = false;
};

inline Parsed Parse(const std::string& s) {
    Parsed p;
    std::size_t i = 0;
    const std::size_t n = s.size();

    if (i < n && (s[i] == 'v' || s[i] == 'V')) ++i;   // tolerate v1.2.3 tags

    auto readInt = [&](int& out) -> bool {
        const std::size_t start = i;
        long value = 0;
        while (i < n && s[i] >= '0' && s[i] <= '9') {
            value = value * 10 + (s[i] - '0');
            if (value > 100000000) return false;      // garbage in, Unknown out
            ++i;
        }
        if (i == start) return false;
        out = static_cast<int>(value);
        return true;
    };

    if (!readInt(p.major)) return p;
    if (i >= n || s[i] != '.') return p;
    ++i;
    if (!readInt(p.minor)) return p;
    if (i >= n || s[i] != '.') return p;
    ++i;
    if (!readInt(p.patch)) return p;

    // "X.Y.Z-N-gHASH": the commit count distinguishes builds of the same tag.
    if (i < n && s[i] == '-') {
        ++i;
        if (!readInt(p.commits)) return p;
    }
    p.ok = true;
    return p;
}

inline Order Compare(const std::string& a, const std::string& b) {
    const Parsed pa = Parse(a);
    const Parsed pb = Parse(b);
    if (!pa.ok || !pb.ok) return Order::Unknown;

    if (pa.major != pb.major) return pa.major < pb.major ? Order::Older : Order::Newer;
    if (pa.minor != pb.minor) return pa.minor < pb.minor ? Order::Older : Order::Newer;
    if (pa.patch != pb.patch) return pa.patch < pb.patch ? Order::Older : Order::Newer;
    if (pa.commits != pb.commits) {
        return pa.commits < pb.commits ? Order::Older : Order::Newer;
    }
    return Order::Same;
}

// Exact same bytes were built: the only safe "nothing to do" test.
inline bool SameBuild(const std::string& a, const std::string& b) { return a == b; }

inline const char* OrderName(Order o) {
    switch (o) {
        case Order::Older:   return "older";
        case Order::Same:    return "same-version";
        case Order::Newer:   return "newer";
        default:             return "unknown";
    }
}

}  // namespace BuildVersion
