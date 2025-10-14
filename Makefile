
SOURCE_FILES= \
			  $(SRC_DIR)/Config/Config/ConfigTetraPier.cpp \
			  \
			  $(SRC_DIR)/Utils/Logger/Logger/FileLogger.cpp \
			  \
			  $(SRC_DIR)/TcpServer/TcpServerInit/TcpServerInit.cpp \
			  \
			  $(SRC_DIR)/GameServer/Utils/ClassPreloader/ClassPreloader.cpp \
			  $(SRC_DIR)/GameServer/Engine/GameEngine/Init/GameEngine__Init.cpp \
			  $(SRC_DIR)/GameServer/Engine/Actor/Spawn/Actor__Spawn.cpp \
			  $(SRC_DIR)/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.cpp \
			  $(SRC_DIR)/GameServer/Engine/ServerCommandlet/Main/ServerCommandlet__Main.cpp \
			  $(SRC_DIR)/GameServer/Engine/GameEngine/SpawnServerActors/GameEngine__SpawnServerActors.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/UdpNetDriver/InitListen/UdpNetDriver__InitListen.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/UdpNetDriver/TickDispatch/UdpNetDriver__TickDispatch.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/LowLevelGetRemoteAddress/NetConnection__LowLevelGetRemoteAddress.cpp \
			  $(SRC_DIR)/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.cpp \
			  $(SRC_DIR)/GameServer/Storage/TeamsData/TeamsData.cpp \
			  $(SRC_DIR)/GameServer/IpDrv/NetConnection/LowLevelSend/NetConnection__LowLevelSend.cpp \
			  $(SRC_DIR)/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp \
			  $(SRC_DIR)/GameServer/Engine/ActorChannel/ReceivedBunch/CanExecute/ActorChannel__ReceivedBunch__CanExecute.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPlayerController/IsReadyForStart/TgPlayerController__IsReadyForStart.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgGame/InitGameRepInfo/TgGame__InitGameRepInfo.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.cpp \
			  $(SRC_DIR)/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.cpp \
			  $(SRC_DIR)/GameServer/Engine/Actor/GetOptimizedRepList/Actor__GetOptimizedRepListV2.cpp \
			  $(SRC_DIR)/dllmain.cpp



CC=i686-w64-mingw32-g++
CFLAGS=-pthread -I. -I./lib/detours -I./lib/asio-1.34.2/include -L/usr/i686-w64-mingw32/lib -shared -static -static-libgcc -static-libstdc++ -fpermissive -s -w
LDFLAGS=-lkernel32 -luser32 -ladvapi32 -lws2_32 -lpsapi -lstdc++ -Wl,--allow-multiple-definition
SRC_DIR=src
LIB_DIR=lib
DATA_DIR=data
OBJ_DIR=obj
OUT_DIR=out


# Ensure folders exist
$(shell mkdir -p $(OBJ_DIR) $(OUT_DIR))

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

# Object file mapping
# VERSION_OBJS=$(VERSION_CPP_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
VERSION_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(VERSION_CPP_SRC))

$(info VERSION_CPP_SRC = $(VERSION_CPP_SRC))
$(info VERSION_OBJS    = $(VERSION_OBJS))

VERSION_DEF=$(DATA_DIR)/version.def
VERSION_OUT=$(OUT_DIR)/version.dll

obj/pch.hpp.gch: src/pch.hpp
	i686-w64-mingw32-g++ -std=c++17 -I. -I./lib/detours -I./lib/asio-1.34.2/include -x c++-header src/pch.hpp -o obj/pch.hpp.gch

# Compile any src .cpp -> obj/src/... .o (with PCH)
$(OBJ_DIR)/src/%.o: src/%.cpp obj/pch.hpp.gch
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile any lib .cpp -> obj/lib/... .o (without PCH)
$(OBJ_DIR)/lib/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# Default target
all: $(VERSION_OUT)

# Build version.dll
$(VERSION_OUT): $(VERSION_OBJS) $(VERSION_DEF)
	$(CC) $(CFLAGS) -o $@ $(VERSION_OBJS) $(VERSION_DEF) $(LDFLAGS)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(OUT_DIR)
