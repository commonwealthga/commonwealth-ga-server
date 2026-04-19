#include "src/GameServer/TgGame/TgPawn_Character/SendCharacterSkillMarshal/TgPawn_Character__SendCharacterSkillMarshal.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UTgPawn_Character::SendCharacterSkillMarshal — reliable server RPC, stub at
// 0x109c0dc0. The client fires this from TgUIAgentProfile_Skill.uc when the
// skill panel opens; we respond by asking the control server to push a fresh
// SEND_PLAYER_SKILLS packet down the client's TCP session. Allocations are
// already delivered once at spawn, so this is the refresh/respec path.
void __fastcall TgPawn_Character__SendCharacterSkillMarshal::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();

	if (!Pawn) { LogCallEnd(); return; }

	auto it = GPawnSessions.find((ATgPawn*)Pawn);
	if (it == GPawnSessions.end()) {
		Logger::Log("skills", "[SendCharacterSkillMarshal] pawn=%p has no session\n", Pawn);
		LogCallEnd();
		return;
	}

	nlohmann::json ev;
	ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
	ev["subtype"]      = "skill_request";
	ev["instance_id"]  = IpcClient::GetInstanceId();
	ev["session_guid"] = it->second;
	IpcClient::Send(ev.dump());

	Logger::Log("skills", "[SendCharacterSkillMarshal] requested push for guid=%s\n",
		it->second.c_str());

	LogCallEnd();
}
