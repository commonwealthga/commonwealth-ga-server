
SOURCE_FILES= \
			  $(SRC_DIR)/Config/Config.cpp \
			  \
			  $(SRC_DIR)/Utils/Logger/Logger/FileLogger.cpp \
			  $(SRC_DIR)/Utils/CommandLineParser/CommandLineParser.cpp \
			  $(SRC_DIR)/Utils/DebugWindow/DebugWindow.cpp \
			  \
			  $(SRC_DIR)/Database/Database.cpp \
			  \
			  $(SRC_DIR)/TcpServer/TcpServerInit/TcpServerInit.cpp \
			  $(SRC_DIR)/TcpServer/TcpEvents/TcpEvents.cpp \
			  \
			  $(SRC_DIR)/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.cpp \
			  $(SRC_DIR)/GameServer/Core/FMallocWindows/Free/FMallocWindows__Free.cpp \
			  $(SRC_DIR)/GameServer/Utils/ClassPreloader/ClassPreloader.cpp \
			  $(SRC_DIR)/GameServer/Engine/GameEngine/Init/GameEngine__Init.cpp \
			  $(SRC_DIR)/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.cpp \
			  $(SRC_DIR)/GameServer/Engine/World/BeginPlay/World__BeginPlay.cpp \
			  $(SRC_DIR)/GameServer/Engine/Actor/Spawn/Actor__Spawn.cpp \
			  $(SRC_DIR)/GameServer/Engine/Actor/Tick/Actor__Tick.cpp \
			  $(SRC_DIR)/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.cpp \
			  $(SRC_DIR)/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.cpp \
			  $(SRC_DIR)/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/UdpNetDriver/TickDispatch/UdpNetDriver__TickDispatch.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/LowLevelGetRemoteAddress/NetConnection__LowLevelGetRemoteAddress.cpp \
			  $(SRC_DIR)/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.cpp \
			  $(SRC_DIR)/GameServer/Storage/TeamsData/TeamsData.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/Cleanup/NetConnection__Cleanup.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/CleanupActor/NetConnection__CleanupActor.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/MarshalChannel/MarshalReceived/MarshalChannel__MarshalReceived.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp \
			  $(SRC_DIR)/GameServer/Engine/ActorChannel/ReceivedBunch/CanExecute/ActorChannel__ReceivedBunch__CanExecute.cpp \
			  $(SRC_DIR)/GameServer/Engine/Channel/ReceivedSequencedBunch/Channel__ReceivedSequencedBunch.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/IsReadyForStart/TgPlayerController__IsReadyForStart.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/SetSoundMode/TgPlayerController__SetSoundMode.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/CanPlayerUseVolume/TgPlayerController__CanPlayerUseVolume.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/GetViewTarget/TgPlayerController__GetViewTarget.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/TgFindPlayerStart/TgGame__TgFindPlayerStart.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter__GiveJetpack.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnBotPawn/TgGame__SpawnBotPawn.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/ReviveDefendersTimer/TgGame__ReviveDefendersTimer.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/MissionTimeRemaining/TgGame__MissionTimeRemaining.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SendMissionTimerEvent/TgGame__SendMissionTimerEvent.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/SwapAttachedDeviceMaterials/TgPawn__SwapAttachedDeviceMaterials.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.cpp \
			  $(SRC_DIR)/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepListV2.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/SpawnBot/TgBotFactory__SpawnBot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/SpawnWave/TgBotFactory__SpawnWave.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnBot/TgGame__SpawnBot.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDevice/HasEnoughPowerPool/TgDevice__HasEnoughPowerPool.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgDevice/HasMinimumPowerPool/TgDevice__HasMinimumPowerPool.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.cpp \
			  $(SRC_DIR)/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.cpp \
			  $(SRC_DIR)/GameServer/Misc/CGameClient/SendMapRandomSMSettingsMarshal/CGameClient__SendMapRandomSMSettingsMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.cpp \
			  $(SRC_DIR)/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmBot/LoadBotSpawnTableMarshal/CAmBot__LoadBotSpawnTableMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmDeviceModel/LoadDeviceMarshal/CAmDeviceModel__LoadDeviceMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmDeviceModel/LoadDeviceModeMarshal/CAmDeviceModel__LoadDeviceModeMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.cpp \
			  $(SRC_DIR)/GameServer/Misc/CAmOmegaVolume/LoadOmegaVolumeMarshal/CAmOmegaVolume__LoadOmegaVolumeMarshal.cpp \
			  $(SRC_DIR)/dllmain.cpp

SOURCE_FILES_CLIENT= \
			  $(SRC_DIR)/Utils/Logger/Logger/FileLogger.cpp \
			  $(SRC_DIR)/dllmainclient.cpp

# JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)
# MAKEFLAGS += -j$(JOBS)
MAKEFLAGS += -j4
CC=i686-w64-mingw32-g++
CFLAGS=-pthread -I. -I./lib/detours -I./lib/asio-1.34.2/include -I./lib/sqlite3 -L/usr/i686-w64-mingw32/lib -shared -static -static-libgcc -static-libstdc++ -fpermissive -s -w
LDFLAGS=-lkernel32 -luser32 -ladvapi32 -lws2_32 -lpsapi -lstdc++ -lgdi32 -Wl,--allow-multiple-definition
SRC_DIR=src
LIB_DIR=lib
DATA_DIR=data
OBJ_DIR=obj
OUT_DIR=out
OUT_CLIENT_DIR=out/client

# Ensure folders exist
$(shell mkdir -p $(OBJ_DIR) $(OUT_DIR) $(OUT_CLIENT_DIR))

SDK_CPP_SRC=$(SRC_DIR)/SDK/SdkHeaders.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/Core_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/Engine_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/GameFramework_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/UnrealScriptTest_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/TgGame_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/TgNetDrv_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/TgClient_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/XAudio2_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/ALAudio_functions.cpp \
			$(SRC_DIR)/SDK/SDK_HEADERS/WinDrv_functions.cpp

VERSION_CPP_SRC=$(LIB_DIR)/detours/modules.cpp \
				$(LIB_DIR)/detours/disasm.cpp \
				$(LIB_DIR)/detours/detours.cpp \
				$(SDK_CPP_SRC) \
				$(SOURCE_FILES)

VERSION_CPP_CLIENT_SRC=$(LIB_DIR)/detours/modules.cpp \
				$(LIB_DIR)/detours/disasm.cpp \
				$(LIB_DIR)/detours/detours.cpp \
				$(SDK_CPP_SRC) \
				$(SOURCE_FILES_CLIENT)

# Object file mapping
# VERSION_OBJS=$(VERSION_CPP_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

VERSION_CPP_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(VERSION_CPP_SRC))
VERSION_OBJS := $(VERSION_CPP_OBJS) $(OBJ_DIR)/lib/sqlite3/sqlite3.o
VERSION_CLIENT_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/client/%.o,$(VERSION_CPP_CLIENT_SRC))

$(info VERSION_CPP_SRC = $(VERSION_CPP_SRC))
$(info VERSION_OBJS    = $(VERSION_OBJS))

VERSION_DEF=$(DATA_DIR)/version.def
VERSION_OUT=$(OUT_DIR)/version.dll
VERSION_CLIENT_OUT=$(OUT_CLIENT_DIR)/version.dll

obj/pch.hpp.gch: src/pch.hpp
	i686-w64-mingw32-g++ -std=c++17 -I. -I./lib/detours -I./lib/asio-1.34.2/include -x c++-header src/pch.hpp -o obj/pch.hpp.gch


# Compile any src .cpp -> obj/src/... .o (with PCH)
$(OBJ_DIR)/src/%.o: src/%.cpp obj/pch.hpp.gch
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/client/src/%.o: src/%.cpp obj/pch.hpp.gch
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile any lib .cpp -> obj/lib/... .o (without PCH)
$(OBJ_DIR)/lib/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/client/lib/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/lib/sqlite3/sqlite3.o: lib/sqlite3/sqlite3.c
	@mkdir -p $(dir $@)
	i686-w64-mingw32-gcc -O2 -I./lib/sqlite3 -c $< -o $@

# Default target
all: $(VERSION_OUT) $(VERSION_CLIENT_OUT)

# Build version.dll
$(VERSION_OUT): $(VERSION_OBJS) $(VERSION_DEF)
	$(CC) $(CFLAGS) -o $@ $(VERSION_OBJS) $(VERSION_DEF) $(LDFLAGS)

$(VERSION_CLIENT_OUT): $(VERSION_CLIENT_OBJS) $(VERSION_DEF)
	$(CC) $(CFLAGS) -o $@ $(VERSION_CLIENT_OBJS) $(VERSION_DEF) $(LDFLAGS)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(OBJ_CLIENT_DIR) $(OUT_DIR) $(OUT_CLIENT_DIR)

