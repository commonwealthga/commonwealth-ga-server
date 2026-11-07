#include "src/GameServer/Stats/EscortState/EscortState.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <vector>

namespace EscortState {
namespace {

bool IsEscort(ATgMissionObjective* Obj) {
    return Obj && ObjectClassCache::ClassNameContains((UObject*)Obj, "TgMissionObjective_Escort");
}

USequenceEvent* FindCommander(ATgMissionObjective* Obj) {
    TArray<USequenceEvent*>& evts = Obj->GeneratedEvents;
    if (!evts.Data) return nullptr;
    for (int i = 0; i < evts.Num(); i++)
        if (evts.Data[i] && ObjectClassCache::ClassNameContains((UObject*)evts.Data[i], "TgSeqEvent_ObjectiveMatineeCommander"))
            return evts.Data[i];
    return nullptr;
}

// First SeqAct_Interp reachable from the commander's output links (a few
// hops, in case the level designer put a gate/delay in between).
USeqAct_Interp* FindInterp(USequenceOp* Start) {
    std::vector<USequenceOp*> frontier{Start}, seen{Start};
    for (int depth = 0; depth < 4 && !frontier.empty(); depth++) {
        std::vector<USequenceOp*> next;
        for (USequenceOp* Op : frontier) {
            TArray<FSeqOpOutputLink>& outs = Op->OutputLinks;
            for (int o = 0; outs.Data && o < outs.Num(); o++) {
                TArray<FSeqOpOutputInputLink>& links = outs.Data[o].Links;
                for (int l = 0; links.Data && l < links.Num(); l++) {
                    USequenceOp* To = links.Data[l].LinkedOp;
                    if (!To) continue;
                    bool known = false;
                    for (USequenceOp* s : seen) if (s == To) { known = true; break; }
                    if (known) continue;
                    if (ObjectClassCache::ClassNameContains((UObject*)To, "SeqAct_Interp")) return (USeqAct_Interp*)To;
                    seen.push_back(To);
                    next.push_back(To);
                }
            }
        }
        frontier.swap(next);
    }
    return nullptr;
}

USeqAct_Interp* InterpFor(ATgMissionObjective* Obj) {
    if (!IsEscort(Obj)) return nullptr;
    USequenceEvent* Cmd = FindCommander(Obj);
    return Cmd ? FindInterp((USequenceOp*)Cmd) : nullptr;
}

}  // namespace

nlohmann::json CollectMatinee(TArray<ATgMissionObjective*>& arr) {
    nlohmann::json out = nlohmann::json::array();
    for (int i = 0; arr.Data && i < arr.Num(); i++) {
        USeqAct_Interp* Interp = InterpFor(arr.Data[i]);
        out.push_back(Interp ? Interp->Position : -1.0f);
    }
    return out;
}

nlohmann::json CollectRobot(TArray<ATgMissionObjective*>& arr) {
    for (int i = 0; arr.Data && i < arr.Num(); i++) {
        if (!IsEscort(arr.Data[i])) continue;
        AActor* Robot = ((ATgMissionObjective_Escort*)arr.Data[i])->r_AttachedActor;
        if (!Robot) continue;
        return nlohmann::json{{"x", Robot->Location.X}, {"y", Robot->Location.Y}, {"z", Robot->Location.Z},
                              {"yaw", Robot->Rotation.Yaw}};
    }
    return nullptr;
}

int ApplyMatinee(TArray<ATgMissionObjective*>& arr, const nlohmann::json& positions) {
    if (!positions.is_array() || !arr.Data || (int)positions.size() != arr.Num()) return 0;
    // Several escort objectives (track segments) can drive the SAME matinee;
    // each matinee is seeked once, to the first saved value (all entries for
    // one matinee carry the same Position).
    std::vector<USeqAct_Interp*> done;
    int moved = 0;
    for (int i = 0; i < arr.Num(); i++) {
        if (!positions[i].is_number()) continue;
        const float want = positions[i].get<float>();
        if (want < 0.0f) continue;
        ATgMissionObjective* Obj = arr.Data[i];
        USeqAct_Interp* Interp = InterpFor(Obj);
        if (!Interp) continue;
        bool seen = false;
        for (USeqAct_Interp* d : done) if (d == Interp) { seen = true; break; }
        if (seen) continue;
        done.push_back(Interp);
        const float before = Interp->Position;
        if (Interp->GroupInst.Num() > 0) {
            Interp->SetPosition(want, 1);
        } else {
            // Matinee never started on this host: let the objective's own
            // commander start + seek it.
            USequenceEvent* Cmd = FindCommander(Obj);
            if (Cmd) ((UTgSeqEvent_ObjectiveMatineeCommander*)Cmd)->SetPosition(want);
        }
        AActor* Robot = ((ATgMissionObjective_Escort*)Obj)->r_AttachedActor;
        Logger::Log("restore", "[Restore] escort %d matinee %.2f -> %.2f (want %.2f, groups %d) robot %.0f,%.0f,%.0f\n",
            i, before, Interp->Position, want, Interp->GroupInst.Num(),
            Robot ? Robot->Location.X : 0.0f, Robot ? Robot->Location.Y : 0.0f, Robot ? Robot->Location.Z : 0.0f);
        moved++;
    }
    return moved;
}

}  // namespace EscortState
