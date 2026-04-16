#include "src/GameServer/TgGame/TgPawn/TGPostRenderFor/TgPawn__TGPostRenderFor.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Rate-limit: avoid spamming log every frame for every pawn. Only log pawns
// that have pending damage-display entries (count > 0), and only once per
// (pawn, count) transition.
static int g_lastCount[256] = {0};
static void* g_lastPawn[256] = {0};
static int g_slotIdx = 0;

void __fastcall TgPawn__TGPostRenderFor::Call(ATgPawn* Pawn, void* edx, void* PC, void* Canvas,
	float cpX, float cpY, float cpZ, float cdX, float cdY, float cdZ)
{
	if (Pawn) {
		int count = *(int*)((char*)Pawn + 0xFFC);
		unsigned flagsB4 = *(unsigned*)((char*)Pawn + 0xB4);
		unsigned flags3D8 = *(unsigned*)((char*)Pawn + 0x3D8);
		void* rOwner = *(void**)((char*)Pawn + 0x106C);

		// Only log when pawn has damage entries (what we care about)
		// and limit repeats: only log if pawn or count changed for that slot.
		if (count > 0) {
			bool seen = false;
			for (int i = 0; i < 256; i++) {
				if (g_lastPawn[i] == Pawn && g_lastCount[i] == count) { seen = true; break; }
			}
			if (!seen) {
				g_lastPawn[g_slotIdx] = Pawn;
				g_lastCount[g_slotIdx] = count;
				g_slotIdx = (g_slotIdx + 1) & 255;

				Logger::Log("damage-info",
					"TGPostRenderFor: pawn=%p count=%d flags+0xB4=0x%08x flags+0x3D8=0x%08x r_Owner=%p\n",
					Pawn, count, flagsB4, flags3D8, rOwner);
			}
		}
	}
	CallOriginal(Pawn, edx, PC, Canvas, cpX, cpY, cpZ, cdX, cdY, cdZ);
}
