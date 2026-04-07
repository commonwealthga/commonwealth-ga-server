#include "src/pch.hpp"

#include "src/GameServer/Engine/LaunchEngineLoop/ConstructCommandletObject/ConstructCommandletObject.hpp"
#include "src/Utils/Logger/Logger.hpp"

void ConstructCommandletObject::Call(void* param_1, int param_2, void* param_3, void* param_4, int param_5, int param_6, void* param_7, void* param_8, void* param_9) {

	LogCallBegin();

	for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
		UObject* obj = UObject::GObjObjects()->Data[i];
		if (obj) {
			char* className = obj->Class->GetFullName();
			if (strcmp(className, "Class Engine.ServerCommandlet") == 0) {
				param_1 = obj;
				return;
			}
		}
	}

	ConstructCommandletObject::CallOriginal(param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8, param_9);

	LogCallEnd();
}
