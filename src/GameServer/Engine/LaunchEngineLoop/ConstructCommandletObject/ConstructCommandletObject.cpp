#include "src/pch.hpp"

#include "src/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.hpp"

void ConstructCommandletObject::Call(void* param_1, int param_2, void* param_3, void* param_4, int param_5, int param_6, void* param_7, void* param_8, void* param_9) {

	for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
		if (UObject::GObjObjects()->Data[i] && UObject::GObjObjects()->Data[i]->IsA(UServerCommandlet::StaticClass())) {
			param_1 = UObject::GObjObjects()->Data[i];
			return;
		}
	}

	ConstructCommandletObject::CallOriginal(param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8, param_9);
}

