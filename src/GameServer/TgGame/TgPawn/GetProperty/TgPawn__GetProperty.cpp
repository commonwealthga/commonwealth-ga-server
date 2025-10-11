#include "src/GameServer/TgGame/TgPawn/GetProperty/TgPawn__GetProperty.hpp"

UTgProperty* TgPawn__GetProperty::Call(ATgPawn* Pawn, void* edx, int PropertyId) {
	for (int i = 0; i < Pawn->s_Properties.Num(); i++) {
		UTgProperty* prop = Pawn->s_Properties.Data[i];
		if (prop->m_nPropertyId == PropertyId) {
			// LogToFile("C:\\mylog.txt", "Pawn::GetProperty(%d) FOUND", PropertyId);
			return prop;
		}
	}
	// LogToFile("C:\\mylog.txt", "Pawn::GetProperty(%d) NOT FOUND", PropertyId);

	return CallOriginal(Pawn, edx, PropertyId);
}

