#include "src/GameServer/TgAssemblyMisc/LoadAssetRefs/TgAssemblyMisc__LoadAssetRefs.hpp"
#include "src/Utils/Logger/Logger.hpp"

thread_local int TgAssemblyMisc__LoadAssetRefs::s_nestLevel = 0;

// Best-effort render of a UObject*'s GetFullName. The caller here passes
// whatever the specific caller hardcoded — typically a UTgDeviceForm /
// ATgRepInfo_Player / ATgPlayerController, all UObjects. If param_2 isn't
// a UObject the vtable call would crash; we guard by returning "<?>" when
// the pointer is null or obviously garbage.
static const char* SafeFullName(void* obj) {
	if (!obj) return "<null>";
	// Reject small-int values that can't be pointers (guards against the
	// same cfg+0x10-style misinterpretation that bit us earlier).
	if ((uintptr_t)obj < 0x10000) return "<smallint>";
	UObject* u = (UObject*)obj;
	const char* n = u->GetFullName();
	return n ? n : "<?>";
}

int __fastcall TgAssemblyMisc__LoadAssetRefs::Call(void* pThis, void* edx, int param_1, void* param_2, int param_3) {
	// Snapshot the interesting fields on `this` before calling original —
	// the state dword at +0x10 mutates inside (0/1 → 2 → 3) and we want to
	// see the pre-call value for diagnosis.
	int preState   = pThis ? *(int*)((char*)pThis + 0x10)  : -1;
	void* refList  = pThis ? *(void**)((char*)pThis + 0x14) : nullptr;
	void* refOwner = pThis ? *(void**)((char*)pThis + 0x18) : nullptr;

	// Classify the caller via the hardcoded param_1 each caller passes.
	const char* caller = "?";
	switch (param_1) {
		case 0x10a97300: caller = "UTgDeviceForm::Init"; break;
		case 0x10a2b020: caller = "FUN_10a2c0a0_wrapper"; break;
		default:         caller = "Other (ClientLoadDevices / UpdateDeviceAssetRefs / other)";
	}

	Logger::Log("asset_load",
		"[LoadAssetRefs] ENTER this=0x%p caller='%s'(0x%08x) context=0x%p name='%s' state=%d refList=0x%p refOwner=0x%p\n",
		pThis, caller, (unsigned)param_1, param_2,
		SafeFullName(param_2), preState, refList, refOwner);

	s_nestLevel++;
	int ret = -1;
	ret = CallOriginal(pThis, edx, param_1, param_2, param_3);
	s_nestLevel--;

	// When called from UTgDeviceForm::Init (param_1 == 0x10a97300) and the
	// loader completed successfully (return==3), mirror the write that the
	// cached branch in FUN_10a99190 does:
	//     form+0x10C = refContainer+0x18
	// The native loader populates refContainer+0x18 (m_pObjectReferencer)
	// but does NOT touch form+0x10C ("m_AssetReference"); the caller then
	// asserts `form+0x10c != 0` and tripping the assertion triggers
	// CrashProgram("m_AssetReference", "TgDeviceForm.cpp", 0x81) at
	// 0x10a99282. The assertion is what the cached path would have set on a
	// prior successful load — we set it ourselves since our path skips the
	// cache state update.
	if (ret == 3 && param_1 == 0x10a97300 && param_2 && pThis) {
		void* referencer = *(void**)((char*)pThis + 0x18);
		if (referencer) {
			void** form_m_AssetReference = (void**)((char*)param_2 + 0x10C);
			void* prev = *form_m_AssetReference;
			*form_m_AssetReference = referencer;
			Logger::Log("asset_load",
				"[LoadAssetRefs] SET form+0x10C (m_AssetReference): form=0x%p  was=0x%p  now=0x%p\n",
				param_2, prev, referencer);
		} else {
			Logger::Log("asset_load",
				"[LoadAssetRefs] WARNING state=3 but referencer (this+0x18)==null; form+0x10C not updated\n");
		}
	}

	Logger::Log("asset_load",
		"[LoadAssetRefs] EXIT  this=0x%p caller='%s' → state=%d (ret=%d)\n",
		pThis, caller,
		pThis ? *(int*)((char*)pThis + 0x10) : -1, ret);

	return ret;
}
