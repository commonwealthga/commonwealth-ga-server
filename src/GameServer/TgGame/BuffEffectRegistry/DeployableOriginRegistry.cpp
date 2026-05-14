#include "src/GameServer/TgGame/BuffEffectRegistry/DeployableOriginRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <map>

namespace DeployableOriginRegistry {

namespace {
	std::map<ATgDeployable*, Origin> g_table;
	std::map<UTgEffectGroup*, UTgDeviceFire*> g_templateOwners;
	std::map<UTgEffectGroup*, Origin> g_cloneOrigins;
}

void Register(ATgDeployable* deployable, int spawnDevInstId, int spawnDevSkillId) {
	if (!deployable) return;
	g_table[deployable] = { spawnDevInstId, spawnDevSkillId };
	Logger::Log("deployable",
		"[DeployableOriginRegistry] register dep=0x%p spawnDevInst=%d spawnDevSkill=%d\n",
		deployable, spawnDevInstId, spawnDevSkillId);
}

Origin Lookup(ATgDeployable* deployable) {
	if (!deployable) return { 0, 0 };
	auto it = g_table.find(deployable);
	return (it != g_table.end()) ? it->second : Origin{ 0, 0 };
}

void Unregister(ATgDeployable* deployable) {
	if (!deployable) return;
	g_table.erase(deployable);
}

void NoteTemplateOwner(UTgEffectGroup* templateEg, UTgDeviceFire* fireMode) {
	if (!templateEg || !fireMode) return;
	g_templateOwners[templateEg] = fireMode;
}

UTgDeviceFire* GetTemplateOwner(UTgEffectGroup* templateEg) {
	if (!templateEg) return nullptr;
	auto it = g_templateOwners.find(templateEg);
	return (it != g_templateOwners.end()) ? it->second : nullptr;
}

void RegisterClone(UTgEffectGroup* clone, int spawnDevInstId, int spawnDevSkillId) {
	if (!clone) return;
	g_cloneOrigins[clone] = { spawnDevInstId, spawnDevSkillId };
}

Origin LookupClone(UTgEffectGroup* clone) {
	if (!clone) return { 0, 0 };
	auto it = g_cloneOrigins.find(clone);
	return (it != g_cloneOrigins.end()) ? it->second : Origin{ 0, 0 };
}

void UnregisterClone(UTgEffectGroup* clone) {
	if (!clone) return;
	g_cloneOrigins.erase(clone);
}

}  // namespace DeployableOriginRegistry
