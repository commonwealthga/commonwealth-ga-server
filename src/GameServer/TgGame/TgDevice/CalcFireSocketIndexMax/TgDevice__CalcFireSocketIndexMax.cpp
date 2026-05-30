#include "src/GameServer/TgGame/TgDevice/CalcFireSocketIndexMax/TgDevice__CalcFireSocketIndexMax.hpp"
#include "src/Database/SocketCycle/SocketCycle.hpp"
#include "src/Utils/Logger/Logger.hpp"

// One-shot diagnostic: dump Pawn->Mesh state for every pawn whose
// CalcFireSocketIndexMax we override (i.e. bots with multi-socket cycles
// in our DB cache). We need to know whether Pawn->Mesh is null
// server-side or if it's actually live (with Sockets loaded). The
// answer decides whether we can call GetSocketWorldLocationAndRotation
// directly from C++ vs. needing an entirely different data path
// (UPK extraction / GObjObjects walk).
//
// Channel: "socketdiag". Dedup per pawn pointer so each bot logs once.
static std::unordered_set<uintptr_t> s_meshLoggedPawns;

void __fastcall TgDevice__CalcFireSocketIndexMax::Call(ATgDevice* Device, void* edx) {
    CallOriginal(Device, edx);

    if (!Device) return;
    if (Device->m_nSocketMax > 1) return;  // original lookup succeeded — don't override

    APawn* Instigator = Device->Instigator;
    if (!Instigator) return;

    int count = SocketCycle::LookupOriginSocketCount(
        reinterpret_cast<ATgPawn*>(Instigator),
        (int)Device->r_eEquippedAt);

    if (count > 1) {
        Device->m_nSocketMax = count;

        // ── Diagnostic (one-shot per pawn) ─────────────────────────────
        if (Logger::IsChannelEnabled("socketdiag")) {
            uintptr_t key = (uintptr_t)Instigator;
            if (s_meshLoggedPawns.insert(key).second) {
                // Copy both names to std::string immediately — GetFullName
                // shares a static buffer; the second call clobbers the first.
                const char* pawnRaw = Instigator->GetFullName();
                std::string pawnName = pawnRaw ? pawnRaw : "(null)";
                void* mesh = (void*)Instigator->Mesh;
                void* skel = mesh ? (void*)Instigator->Mesh->SkeletalMesh : nullptr;
                int   socketsCount = skel ? Instigator->Mesh->SkeletalMesh->Sockets.Count : -1;
                const char* skelRaw = skel
                    ? Instigator->Mesh->SkeletalMesh->GetFullName()
                    : nullptr;
                std::string skelName = skelRaw ? skelRaw : "(null)";
                Logger::Log("socketdiag",
                    "[MeshState] pawn=%s slot=%d sMax=%d\n"
                    "  Pawn->Mesh=%p  Mesh->SkeletalMesh=%p  SkeletalMesh=%s\n"
                    "  Sockets.Count=%d (-1 means SkeletalMesh is null)\n",
                    pawnName.c_str(),
                    (int)Device->r_eEquippedAt,
                    Device->m_nSocketMax,
                    mesh, skel, skelName.c_str(),
                    socketsCount);

                // If sockets exist, also dump the first few names so we
                // can confirm they really say "WSO_Origin_NN" at runtime
                // (per the user's UPK inspection).
                if (skel && socketsCount > 0) {
                    int n = socketsCount < 8 ? socketsCount : 8;
                    auto& arr = Instigator->Mesh->SkeletalMesh->Sockets;
                    for (int i = 0; i < n; i++) {
                        USkeletalMeshSocket* sock = arr.Data[i];
                        if (!sock) continue;
                        const char* sockName = sock->GetFullName();
                        Logger::Log("socketdiag",
                            "  Socket[%d]=%s\n",
                            i, sockName ? sockName : "(null)");
                    }
                }
            }
        }
        // ── end diagnostic ─────────────────────────────────────────────
    }
}
