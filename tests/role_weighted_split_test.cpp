// Standalone, host-compiled (x86_64) sanity test for RoleWeightedSplit.
// NOT part of the server build. Run manually from the repo root:
//
//   g++ -std=c++17 -I. tests/role_weighted_split_test.cpp \
//       src/ControlServer/MatchmakingService/RoleWeightedSplit.cpp \
//       -o /tmp/rws_test && /tmp/rws_test
//
// Expected output ends with: "ALL TESTS PASSED".

#include "src/ControlServer/MatchmakingService/RoleWeightedSplit.hpp"

#include <cassert>
#include <cstdio>
#include <map>
#include <vector>

using RoleWeightedSplit::PlayerSlot;
using RoleWeightedSplit::TeamState;
using RoleWeightedSplit::RosterEntry;

namespace {
constexpr uint32_t A = RoleWeightedSplit::kProfileAssault;   // 680
constexpr uint32_t M = RoleWeightedSplit::kProfileMedic;     // 567
constexpr uint32_t R = RoleWeightedSplit::kProfileRecon;     // 681
constexpr uint32_t B = RoleWeightedSplit::kProfileRobotics;  // 679

// Count, per class, how many landed on tf1 vs tf2 in a batch assignment.
std::map<uint32_t, std::pair<int,int>>
ClassSplit(const std::vector<PlayerSlot>& players,
           const std::unordered_map<std::string,int>& asn) {
    std::map<uint32_t, std::pair<int,int>> out;
    for (const auto& p : players) {
        int tf = asn.at(p.guid);
        if (tf == 1) out[p.profile_id].first++;
        else         out[p.profile_id].second++;
    }
    return out;
}

std::vector<PlayerSlot> MakeSlots(const std::vector<uint32_t>& classes) {
    std::vector<PlayerSlot> v;
    for (size_t i = 0; i < classes.size(); ++i)
        v.push_back({"g" + std::to_string(i), classes[i]});
    return v;
}
} // namespace

int main() {
    // 1) All classes even -> every class splits exactly evenly.
    {
        auto slots = MakeSlots({A,A,R,R,M,M,B,B});
        auto asn   = RoleWeightedSplit::ComputeBatchAssignment(slots, TeamState{}, TeamState{});
        auto split = ClassSplit(slots, asn);
        for (uint32_t c : {A,R,M,B}) {
            assert(split[c].first == 1 && split[c].second == 1);
        }
        printf("[ok] all-even -> 1/1 per class\n");
    }

    // 2) Stacked assault vs stacked recon, equal sizes -> classes get split.
    //    6 assault + 6 recon must end 3/3 each (the bug this fixes).
    {
        auto slots = MakeSlots({A,A,A,A,A,A,R,R,R,R,R,R});
        auto asn   = RoleWeightedSplit::ComputeBatchAssignment(slots, TeamState{}, TeamState{});
        auto split = ClassSplit(slots, asn);
        assert(split[A].first == 3 && split[A].second == 3);
        assert(split[R].first == 3 && split[R].second == 3);
        printf("[ok] 6 assault / 6 recon -> 3/3 each\n");
    }

    // 3) Odd robotics (3) -> heal-balancing puts the extra on ATTACK (tf1):
    //    2*0.75 vs 1*0.85 (diff 0.65) beats 1*0.75 vs 2*0.85 (diff 0.95).
    {
        auto slots = MakeSlots({B,B,B});
        auto asn   = RoleWeightedSplit::ComputeBatchAssignment(slots, TeamState{}, TeamState{});
        auto split = ClassSplit(slots, asn);
        assert(split[B].first == 2 && split[B].second == 1);
        printf("[ok] 3 robotics -> 2 attack / 1 defense (heal-balanced)\n");
    }

    // 4) ComputeRebalanceDelta: already-balanced roster -> no moves.
    {
        std::vector<RosterEntry> roster = {
            {"a",A,1},{"b",A,2},{"c",R,1},{"d",R,2},
        };
        auto delta = RoleWeightedSplit::ComputeRebalanceDelta(roster);
        assert(delta.empty());
        printf("[ok] balanced roster -> empty delta\n");
    }

    // 5) ComputeRebalanceDelta: 4v4 class skew (3 assault/1 recon vs 1/3) ->
    //    exactly one assault and one recon swap (2 moves, sizes preserved).
    {
        std::vector<RosterEntry> roster = {
            {"a",A,1},{"b",A,1},{"c",A,1},{"d",R,1},
            {"e",A,2},{"f",R,2},{"g",R,2},{"h",R,2},
        };
        auto delta = RoleWeightedSplit::ComputeRebalanceDelta(roster);
        // One A moves 1->2 and one R moves 2->1 (or symmetric). Net 2 moves.
        assert(delta.size() == 2);
        printf("[ok] 4v4 class skew -> 2 moves\n");
    }

    // 6) ComputeRebalanceDelta: 5v3 size imbalance -> exactly one move.
    {
        std::vector<RosterEntry> roster = {
            {"a",A,1},{"b",A,1},{"c",R,1},{"d",R,1},{"e",M,1},
            {"f",A,2},{"g",R,2},{"h",M,2},
        };
        auto delta = RoleWeightedSplit::ComputeRebalanceDelta(roster);
        assert(delta.size() == 1);
        for (const auto& m : delta) assert(m.second == 2); // moved onto the small team
        printf("[ok] 5v3 -> 1 move onto the small team\n");
    }

    printf("ALL TESTS PASSED\n");
    return 0;
}
