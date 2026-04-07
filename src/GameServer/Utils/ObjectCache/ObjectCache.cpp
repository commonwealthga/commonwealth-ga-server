#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"

std::unordered_map<std::string, UObject*> ObjectCache::ByFullName;
std::unordered_map<std::string, std::vector<UObject*>> ObjectCache::ByClassName;
int ObjectCache::LastIndex = 0;

void ObjectCache::ScanForward(const char* targetFullName) {
	TArray<UObject*>* objects = UObject::GObjObjects();
	if (!objects) return;

	int count = objects->Count;

	for (int i = LastIndex; i < count; i++) {
		UObject* obj = objects->Data[i];
		if (!obj) continue;

		char* fullName = obj->GetFullName();
		std::string fullNameStr(fullName);

		ByFullName[fullNameStr] = obj;

		// Also index by class name (skip Default__ objects for instance queries)
		if (strstr(fullName, "Default__") == nullptr) {
			char* className = obj->Class->GetFullName();
			ByClassName[std::string(className)].push_back(obj);
		}

		if (targetFullName && fullNameStr == targetFullName) {
			LastIndex = i + 1;
			return;
		}
	}

	LastIndex = count;
}

UObject* ObjectCache::Find(const char* fullName) {
	// Check cache first
	auto it = ByFullName.find(fullName);
	if (it != ByFullName.end()) return it->second;

	// Cache miss -- scan forward
	if (LastIndex < UObject::GObjObjects()->Count) {
		ScanForward(fullName);

		it = ByFullName.find(fullName);
		if (it != ByFullName.end()) return it->second;
	}

	return nullptr;
}

const std::vector<UObject*>& ObjectCache::FindAllByClass(const char* classFullName) {
	// Must scan to current end to guarantee we have all instances
	if (LastIndex < UObject::GObjObjects()->Count) {
		ScanForward(nullptr);
	}

	auto it = ByClassName.find(classFullName);
	if (it != ByClassName.end()) return it->second;

	static const std::vector<UObject*> empty;
	return empty;
}

std::vector<UObject*> ObjectCache::FindAllByClassSubstr(const char* classSubstr) {
	// Must scan to current end
	if (LastIndex < UObject::GObjObjects()->Count) {
		ScanForward(nullptr);
	}

	std::vector<UObject*> results;
	for (auto& pair : ByClassName) {
		if (pair.first.find(classSubstr) != std::string::npos) {
			results.insert(results.end(), pair.second.begin(), pair.second.end());
		}
	}
	return results;
}
