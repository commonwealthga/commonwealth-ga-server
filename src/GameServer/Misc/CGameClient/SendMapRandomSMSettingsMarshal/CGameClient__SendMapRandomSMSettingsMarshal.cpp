#include "src/GameServer/Misc/CGameClient/SendMapRandomSMSettingsMarshal/CGameClient__SendMapRandomSMSettingsMarshal.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
#include "src/GameServer/Misc/CMarshal/SetArray/CMarshal__SetArray.hpp"
#include "src/GameServer/Misc/CMarshal/SetWcharT/CMarshal__SetWcharT.hpp"
#include "src/GameServer/Misc/HeapAllocator/Allocate/HeapAllocator__Allocate.hpp"
#include "src/GameServer/Misc/CMarshalObject/Create/CMarshalObject__Create.hpp"
#include "src/GameServer/Misc/CMarshalRowSet/Initialize/CMarshalRowSet__Initialize.hpp"
#include "src/GameServer/Misc/CMarshalRowSet/InsertRow/CMarshalRowSet__InsertRow.hpp"
#include "src/GameServer/Misc/CMarshalRow/Initialize/CMarshalRow__Initialize.hpp"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Utils/CommandLineParser/CommandLineParser.hpp"
#include "src/Utils/Logger/Logger.hpp"


void CGameClient__SendMapRandomSMSettingsMarshal::Call(UNetConnection* Connection, void* MarshalPtr) {

	ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, MarshalPtr);
	NetConnection__FlushNet::CallOriginal(Connection);



	// void* NextMarshalPtr = MarshalPtr;
	//
	// int i = 0;
	// do {
	// 	i++;
	// 	Logger::Log("wtf", "NextMarshalPtr - %d: %p\n", i, NextMarshalPtr);
	// 	Logger::DumpMemory("wtf", NextMarshalPtr, 0x1000, 0);
	// 	ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, NextMarshalPtr);
	// 	NextMarshalPtr = *(void**)((char*)NextMarshalPtr + 0x2f4);
	// } while (NextMarshalPtr);

	return;

	// Logger::Log("wtf", "CGameClient__SendMapRandomSMSettingsMarshal\n");
	//
	// Logger::Log("wtf", "%d\n", (int)((int)Connection + 0x4fb4));
	// Logger::Log("wtf", "%d\n", *(int*)((int)Connection + 0x4fb4));
	//
	// uint16_t nRequestId = GA_U::MAP_RANDOMSM_SETTINGS;
	//
	// void** MarshalPtr[8];
	// CMarshalObject__Create::CallOriginal(MarshalPtr);
	// Logger::Log("wtf", "MarshalPtr: %p\n", MarshalPtr);
	//
	// 		// *(void**)((char*)PackageMap) = Globals::Get().PackageMapVtable;
	//
	// *(void**)((char*)MarshalPtr) = CMarshalObject__Create::CMarshal_vftable;
	// // MarshalPtr[0] = CMarshalObject__Create::CMarshal_vftable;
	// Logger::Log("wtf", "MarshalPtr[0]: %p\n", MarshalPtr[0]);
	//
	// *(uint16_t*)((char*)MarshalPtr + 0x26) = nRequestId;
	//
	// ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, MarshalPtr);
	//
	// return;
	//
	//
	// Logger::Log("wtf", "nRequestId: %d\n", nRequestId);
	//
	// void* DataSetPtr = HeapAllocator__Allocate::CallOriginal(0x18);
	//
	// Logger::Log("wtf", "DataSetPtr: %p\n", DataSetPtr);
	// CMarshalRowSet__Initialize::CallOriginal(DataSetPtr);
	//
	// Logger::Log("wtf", "DataSetPtr: %p\n", DataSetPtr);
	//
	//
	// for (int i = 0; i < Names.size(); i++) {
	// 	int Pos = i * 0x18;
	//
	// 	Logger::Log("wtf", "\nRow Pos: %d\n", Pos);
	//
	// 	void* DataRowPtr = HeapAllocator__Allocate::CallOriginal(0x24);
	// 	Logger::Log("wtf", "DataRowPtr: %p\n", DataRowPtr);
	// 	CMarshalRow__Initialize::CallOriginal(DataRowPtr);
	// 	Logger::Log("wtf", "DataRowPtr: %p\n", DataRowPtr);
	//
	// 	std::wstring WideName = CommandLineParser::Utf8ToWide(Names[i]);
	// 	Logger::Log("wtf", "WideName: %ls\n", WideName.c_str());
	//
	// 	CMarshal__SetWcharT::CallOriginal(DataRowPtr, nullptr, GA_T::NAME, const_cast<wchar_t*>(WideName.c_str()));
	// 	Logger::Log("wtf", "DataRowPtr: %p\n", DataRowPtr);
	//
	// 	CMarshalRowSet__InsertRow::CallOriginal(DataSetPtr, nullptr, DataRowPtr);
	// 	Logger::Log("wtf", "DataSetPtr: %p\n", DataSetPtr);
	// }
	//
	// CMarshal__SetArray::CallOriginal(MarshalPtr, nullptr, GA_T::DATA_SET, DataSetPtr);
	// Logger::Log("wtf", "MarshalPtr: %p\n", MarshalPtr);
	//
	// ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, MarshalPtr);
	// Logger::Log("wtf", "DONE\n");
}

