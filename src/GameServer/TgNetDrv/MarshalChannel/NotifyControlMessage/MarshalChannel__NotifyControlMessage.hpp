#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"
#include <unordered_map>
#include <vector>

class MarshalChannel__NotifyControlMessage : public HookBase<
	void(__fastcall*)(void*, void*, UNetConnection*, void*),
	0x109FA6E0,
	MarshalChannel__NotifyControlMessage> {
public:
	static void __fastcall Call(UMarshalChannel* MarshalChannel, void* edx, UNetConnection* Connection, void* InBunch);
	static inline void __fastcall CallOriginal(UMarshalChannel* MarshalChannel, void* edx, UNetConnection* Connection, void* InBunch) {
		LogCallOriginalBegin();
		m_original(MarshalChannel, edx, Connection, InBunch);
		LogCallOriginalEnd();
	};

	static inline std::string GuidToHex(const FGuid& g) {
		char buf[33];
		snprintf(buf, sizeof(buf),
		   "%08X%08X%08X%08X",
		   g.A, g.B, g.C, g.D);
		return std::string(buf);
	};

	static inline std::string SessionGuidToHex(const UUID* id)
	{
		uint32_t d1 = ntohl(id->Data1);
		uint16_t d2 = ntohs(id->Data2);
		uint16_t d3 = ntohs(id->Data3);

		char out[33];
		std::snprintf(
			out,
			sizeof(out),
			"%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
			d1, d2, d3,
			id->Data4[0], id->Data4[1],
			id->Data4[2], id->Data4[3],
			id->Data4[4], id->Data4[5],
			id->Data4[6], id->Data4[7]
		);

		return std::string(out);
	}

	static void HandlePlayerConnected(UNetConnection* Connection, ATgPlayerController* Controller,
		const std::string& session_guid, const std::string& player_name);

	// Fix forced-export GUIDs in the package map to match standalone .upk disk GUIDs.
	static void FixForcedExportGuids(void* PackageMap);

private:
	// Lazily-built map of package name -> full file path for .upk/.u files in CookedPC
	static std::unordered_map<std::string, std::string> s_packageFileMap;
	static bool s_packageFileMapBuilt;
	static void BuildPackageFileMap();
	static bool ReadGuidFromPackageFile(const char* filePath, FGuid& outGuid);
	static bool ReadGuidAndGenerationsFromPackageFile(const char* filePath, FGuid& outGuid, std::vector<int>& outNetObjectCounts);
};

