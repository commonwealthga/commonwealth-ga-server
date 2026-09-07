#include "src/ControlServer/MatchmakingService/DefenderRotation.hpp"

#include <algorithm>
#include <cmath>

namespace DefenderRotation {

namespace {

// Claim order: never defended first, then least recently, then smallest
// defender share, then longest waiting. guid breaks any remaining tie so the
// result never depends on input order.
bool StrongerClaim(const QueuedPlayer& a, const QueuedPlayer& b) {
    if (a.fairness.last_defender_id != b.fairness.last_defender_id)
        return a.fairness.last_defender_id < b.fairness.last_defender_id;
    const double ra = a.fairness.defender_ratio(), rb = b.fairness.defender_ratio();
    if (std::fabs(ra - rb) > 1e-9) return ra < rb;
    if (a.joined_at != b.joined_at) return a.joined_at < b.joined_at;
    return a.session_guid < b.session_guid;
}

}  // namespace

std::unordered_map<std::string, int> Assign(
    const std::vector<const QueuedParty*>& chosen,
    int tf1_n, int tf2_n, bool* out_split) {

    if (out_split) *out_split = false;
    std::unordered_map<std::string, int> out;
    if (chosen.empty()) return out;

    // 1. Rank every member by claim; rank 0 = strongest claim to defend.
    std::vector<const QueuedPlayer*> members;
    for (const QueuedParty* p : chosen)
        for (const auto& m : p->members) members.push_back(&m);
    std::sort(members.begin(), members.end(),
        [](const QueuedPlayer* a, const QueuedPlayer* b) { return StrongerClaim(*a, *b); });
    std::unordered_map<std::string, int> rank;
    for (size_t i = 0; i < members.size(); ++i) rank[members[i]->session_guid] = (int)i;

    auto party_cost = [&](const QueuedParty* p) {
        long long c = 0;
        for (const auto& m : p->members) c += rank[m.session_guid];
        return c;
    };
    auto party_max_rank = [&](const QueuedParty* p) {
        int x = 0;
        for (const auto& m : p->members) x = std::max(x, rank[m.session_guid]);
        return x;
    };

    // 2. Exact, cohesion-preserving choice: the party subset summing to
    //    exactly tf2_n with the strongest total claim. At most 10 parties
    //    (kMaxTotal), so <=1024 masks — exhaustive is exact and cheap.
    const size_t n = chosen.size();
    long long best_cost = 0;
    int best_max = 0;
    uint32_t best_mask = 0;
    bool found = false;
    if (n <= 20) {
        for (uint32_t mask = 1; mask < (1u << n); ++mask) {
            int seats = 0;
            for (size_t i = 0; i < n; ++i)
                if (mask & (1u << i)) seats += (int)chosen[i]->size();
            if (seats != tf2_n) continue;
            long long cost = 0;
            int maxr = 0;
            for (size_t i = 0; i < n; ++i) {
                if (!(mask & (1u << i))) continue;
                cost += party_cost(chosen[i]);
                maxr = std::max(maxr, party_max_rank(chosen[i]));
            }
            // Lowest total claim rank wins; ties by lowest worst-rank, then by
            // lowest mask so the choice never depends on iteration luck.
            if (!found || cost < best_cost || (cost == best_cost && maxr < best_max)) {
                found = true; best_cost = cost; best_max = maxr; best_mask = mask;
            }
        }
    }

    if (found) {
        for (size_t i = 0; i < n; ++i) {
            const int tf = (best_mask & (1u << i)) ? 2 : 1;
            for (const auto& m : chosen[i]->members) out[m.session_guid] = tf;
        }
        return out;
    }

    // 3. No subset fits (design D7). Fill TF2 with whole parties in claim
    //    order while they fit, then spill exactly one party across the
    //    boundary, giving its strongest-claim members the remaining seats.
    if (out_split) *out_split = true;
    std::vector<const QueuedParty*> order = chosen;
    std::stable_sort(order.begin(), order.end(),
        [&](const QueuedParty* a, const QueuedParty* b) {
            const double aa = (double)party_cost(a) / (double)a->size();
            const double bb = (double)party_cost(b) / (double)b->size();
            if (std::fabs(aa - bb) > 1e-9) return aa < bb;
            return a->party_id < b->party_id;
        });

    int rem2 = tf2_n;
    std::vector<const QueuedParty*> deferred;
    for (const QueuedParty* p : order) {
        if ((int)p->size() <= rem2) {
            for (const auto& m : p->members) out[m.session_guid] = 2;
            rem2 -= (int)p->size();
        } else {
            deferred.push_back(p);
        }
    }
    for (const QueuedParty* p : deferred) {
        std::vector<const QueuedPlayer*> ms;
        for (const auto& m : p->members) ms.push_back(&m);
        std::sort(ms.begin(), ms.end(),
            [&](const QueuedPlayer* a, const QueuedPlayer* b) {
                return rank[a->session_guid] < rank[b->session_guid];
            });
        for (const QueuedPlayer* m : ms) {
            if (rem2 > 0) { out[m->session_guid] = 2; rem2--; }
            else            out[m->session_guid] = 1;
        }
    }
    (void)tf1_n;  // TF1 is the complement; the caller guarantees the sizes sum.
    return out;
}

}  // namespace DefenderRotation
