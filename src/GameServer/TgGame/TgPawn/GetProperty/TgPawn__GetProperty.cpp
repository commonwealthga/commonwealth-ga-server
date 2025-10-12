#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"
#include "src/Utils/Logger/Logger.hpp"

UTgProperty* TgPawn__GetProperty::Call(ATgPawn* Pawn, void* edx, int PropertyId) {
	Logger::Log("debug", "MINE TgPawn__GetProperty START\n");
	for (int i = 0; i < Pawn->s_Properties.Num(); i++) {
		UTgProperty* prop = Pawn->s_Properties.Data[i];
		if (prop->m_nPropertyId == PropertyId) {
			// LogToFile("C:\\mylog.txt", "Pawn::GetProperty(%d) FOUND", PropertyId);
			Logger::Log("debug", "MINE TgPawn__GetProperty END\n");
			return prop;
		}
	}
	// LogToFile("C:\\mylog.txt", "Pawn::GetProperty(%d) NOT FOUND", PropertyId);

	UTgProperty* result = CallOriginal(Pawn, edx, PropertyId);

	Logger::Log("debug", "MINE TgPawn__GetProperty END\n");

	return result;
}

