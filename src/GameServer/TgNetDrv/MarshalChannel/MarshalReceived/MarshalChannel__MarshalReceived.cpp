#include "src/GameServer/TgNetDrv/MarshalChannel/MarshalReceived/MarshalChannel__MarshalReceived.hpp"
#include "src/Utils/Logger/Logger.hpp"


void MarshalChannel__MarshalReceived::Call(UMarshalChannel* MarshalChannel, void* edx, void* InBunch) {
	Logger::Log("wtf", "CALLED MarshalChannel__MarshalReceived \n");

	CallOriginal(MarshalChannel, edx, InBunch);
}

