#include "src/GameServer/Utils/FireSockets/FireSockets.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {

	// ── Assembly-data entry points (verified decompiles, see
	//    .planning/2026-06-11-fire-sockets-investigation.md) ────────────────

	// Global CAssemblyManager-derived object (static instance, not a pointer:
	// callers do `MOV ECX, 0x1199f868`).
	void* const kAssemblyManager = (void*)0x1199f868;

	using FnGetMeshModel = void*(__thiscall*)(void*, unsigned int);
	const FnGetMeshModel kGetMeshModel = (FnGetMeshModel)0x109429f0;

	// CAmAssemblyMeshModel socket queries over the FX entry vector (+0x88).
	// Entry slot values are equip points 1..24 (converted from SLOT_VALUE_ID
	// at parse time via the derived manager's vtable+0x18 = FUN_109a1320).
	using FnGetSocketName = void(__thiscall*)(void*, FName*, int, int, int, int, int);
	const FnGetSocketName kGetSocketName = (FnGetSocketName)0x109a3530;
	using FnGetSocketMax = int(__thiscall*)(void*, int, int, int, int);
	const FnGetSocketMax kGetSocketMax = (FnGetSocketMax)0x109a3490;

	// ── Engine entry points ─────────────────────────────────────────────────

	// UObject::LoadPackage — BeginLoad + GetPackageLinker + LoadAllObjects.
	// MUST be gated by FindPackageFile (unresolvable name → engine assert).
	using FnLoadPackage = void*(__cdecl*)(void*, const wchar_t*, unsigned int);
	const FnLoadPackage kLoadPackage = (FnLoadPackage)0x112e53c0;

	// StaticFindObjectFast: (Class|NULL, Outer|NULL, Name.Index, Name.Number,
	// bExactClass, bAnyOuter, excludeFlagsA, excludeFlagsB). Outer==NULL
	// searches the global name hash; bAnyOuter accepts nested objects.
	using FnStaticFindObjectFast = void*(__cdecl*)(void*, void*, int, int, int, int, unsigned int, unsigned int);
	const FnStaticFindObjectFast kStaticFindObjectFast = (FnStaticFindObjectFast)0x112d4e90;

	// GPackageFileCache (pointer global), vtable+0x8 = FindPackageFile.
	void** const kGPackageFileCache = (void**)0x13443f40;

	// ULinkerLoad::CreateExport + the BeginLoad/EndLoad bracket it requires.
	using FnCreateExport = unsigned int(__thiscall*)(void*, int);
	const FnCreateExport kCreateExport = (FnCreateExport)0x11307100;
	using FnBeginLoad = void(__cdecl*)();
	const FnBeginLoad kBeginLoad = (FnBeginLoad)0x112cd710;
	using FnEndLoad = void(__cdecl*)();
	const FnEndLoad kEndLoad = (FnEndLoad)0x112e3fe0;

	// GObjLoaders accessor + count (same pair GetPackageLinker walks).
	using FnGetLoader = char*(__cdecl*)(int);
	const FnGetLoader kGetLoader = (FnGetLoader)0x112ce4a0;
	int* const kGObjLoaderCount = (int*)0x13465a70;

	// UObject::StaticConstructObject(Class, Outer, Name.Index, Name.Number,
	// flagsA, flagsB, Template, GError, ?, InstanceGraph) — arg order from
	// the ConstructObject<T> template wrapper @ 0x109a1040 (disassembled;
	// GError = pointer value at 0x134237d4, transient pkg for Outer==-1).
	using FnStaticConstructObject = void*(__cdecl*)(void*, void*, int, int,
		unsigned int, unsigned int, void*, void*, int, void*);
	const FnStaticConstructObject kStaticConstructObject = (FnStaticConstructObject)0x112e23f0;
	using FnGetTransientPackage = void*(__cdecl*)();
	const FnGetTransientPackage kGetTransientPackage = (FnGetTransientPackage)0x112cd0c0;
	void** const kGError = (void**)0x134237d4;

	// Engine FName ctor (hash-correct, wide): __thiscall(FName* this,
	// const wchar_t*, EFindName=1 FNAME_Add, bSplitName=1) — convention from
	// the CalcFireSocketIndexMax call-site disasm @ 0x10a1dcbd.
	using FnEngineFNameCtor = FName*(__thiscall*)(FName*, const wchar_t*, int, int);
	const FnEngineFNameCtor kEngineFNameCtor = (FnEngineFNameCtor)0x112cc8b0;

	struct EngineFString {
		wchar_t* Data = nullptr;
		int      Count = 0;
		int      Max = 0;
	};

	std::string Narrow(const wchar_t* wide) {
		std::string out;
		for (size_t i = 0; wide && wide[i]; i++) out += (char)wide[i];
		return out;
	}

	std::wstring Widen(const std::string& s) {
		return std::wstring(s.begin(), s.end());
	}

	// Engine-hash FName for a narrow ASCII string. Unlike the SDK FName(char*)
	// ctor (which fake-APPENDS missing names to GNames, bypassing the engine's
	// name hash and shadowing later engine interning), this always agrees with
	// engine-created names.
	FName EngineName(const std::string& s) {
		FName out(0);
		std::wstring w = Widen(s);
		kEngineFNameCtor(&out, w.c_str(), 1 /*FNAME_Add*/, 1);
		return out;
	}

	bool FindPackageFileW(const wchar_t* pkgName, std::string* outResolved) {
		void* cache = *kGPackageFileCache;
		if (!cache) return false;
		using Fn = int(__thiscall*)(void*, const wchar_t*, void*, EngineFString*, int);
		Fn fn = (*(Fn**)cache)[2];
		EngineFString out;
		int ok = fn(cache, pkgName, nullptr, &out, 0);
		if (out.Data) {
			if (outResolved) *outResolved = Narrow(out.Data);
			GAllocator::Free(out.Data);  // engine-allocated FString buffer
		}
		return ok != 0;
	}

	// "ShotOrigin" exists in GNames before any pawn fires — the assembly
	// manager interned it while parsing assembly.dat FX entries at startup.
	FName ShotOriginName() {
		static FName s_name(const_cast<char*>("ShotOrigin"));
		return s_name;
	}

	// CAmAssemblyMeshModel resource FNames (row-init FUN_1094b470):
	FName ModelMeshResName(void* model)             { return *(FName*)((char*)model + 0x18); }  // tag 0x0354
	FName ModelSocketOffsetInfoName(void* model)    { return *(FName*)((char*)model + 0x40); }  // tag 0x048A
	float ModelScale(void* model)                   { return *(float*)((char*)model + 0x4C); }  // tag 0x0454 SCALE

	std::unordered_set<int> s_noSoiAsmIds;                          // no synth possible
	std::unordered_map<int, UTgSocketOffsetInfo*> s_synthByAsmId;   // asm id -> rooted synth SOI (per-asm: scale + anchor lift differ)
	std::unordered_map<std::string, bool> s_pkgFileOk;              // package -> file resolvable
	std::unordered_set<std::string> s_pkgLoadedOnce;                // LoadPackage'd this process

	UClass* SocketOffsetInfoClass() {
		static UClass* s_class = nullptr;
		if (!s_class) {
			// Native registration (GetPrivateStaticClass @ 0x10ab8330) places
			// the class in package "Engine"; the UC script class is TgGame's.
			s_class = ClassPreloader::GetClass("Class Engine.TgSocketOffsetInfo");
			if (!s_class) {
				s_class = ClassPreloader::GetClass("Class TgGame.TgSocketOffsetInfo");
			}
			Logger::Log("soi", "[SOI] class lookup -> %p (%s)\n", (void*)s_class,
				s_class ? "ok" : "NOT FOUND under Engine OR TgGame");
		}
		return s_class;
	}

	// ── Export-level asset resolution (cooked load-context bypass) ──────────
	// ULinkerLoad::CreateExport's FIRST gate skips exports whose ObjectFlags
	// don't intersect the linker's load-context flags (linker+0xFC/+0x100,
	// from GIsClient/GIsServer at linker creation). Client-cooked exports are
	// often client-only; we patch the one export we need (plus its outer
	// chain) with the linker's own context mask and create it directly.

	// FObjectExport entry (ExportMap Data linker+0xD8, Count +0xDC, stride
	// 0x5C — all from the CreateExport decompile).
	struct RawExport {
		int NameIndex;              // +0x00
		int NameNumber;             // +0x04
		int OuterIndex;             // +0x08 (>0: export idx+1; 0: pkg root; <0: import)
		int ClassIndex;             // +0x0C
		int Pad10;                  // +0x10
		int ArchetypeIndex;         // +0x14
		unsigned int FlagsA;        // +0x18
		unsigned int FlagsB;        // +0x1C
		unsigned char Pad20[0x10];  // +0x20
		void* Object;               // +0x30
		unsigned char Pad34[0x28];  // +0x34..0x5B
	};
	static_assert(sizeof(RawExport) == 0x5C, "FObjectExport stride");

	char* FindLinkerForPackage(UObject* pkg) {
		for (int i = 0; i < *kGObjLoaderCount; i++) {
			char* linker = kGetLoader(i);
			if (linker && *(UObject**)(linker + 0x3C) == pkg) return linker;
		}
		return nullptr;
	}

	void* StaticFindTopLevelByName(const FName& name) {
		// cls=NULL (any class), anyOuter=0 -> top-level only: the UPackage,
		// via the engine's always-current name hash. (ObjectCache::Find can
		// NOT see objects created mid-game in recycled GObjObjects slots —
		// forward-only cursor.)
		return kStaticFindObjectFast(nullptr, nullptr, name.Index, 0, 0, 0, 0, 0x400);
	}

	void* ForceCreateExportByName(const std::string& pkgShortName, const FName& objName,
	                              const char* logPath) {
		FName pkgFName = EngineName(pkgShortName);
		UObject* pkg = (UObject*)StaticFindTopLevelByName(pkgFName);
		if (!pkg) {
			Logger::Log("soi", "[SOI] %s: no top-level object '%s' post-load "
				"(LoadPackage failed?)\n", logPath, pkgShortName.c_str());
			return nullptr;
		}
		char* linker = FindLinkerForPackage(pkg);
		if (!linker) {
			Logger::Log("soi", "[SOI] %s: package %s exists but has NO live linker\n",
				logPath, pkgShortName.c_str());
			return nullptr;
		}

		RawExport* exports = *(RawExport**)(linker + 0xD8);
		const int count = *(int*)(linker + 0xDC);
		const unsigned int ctxA = *(unsigned int*)(linker + 0xFC);
		const unsigned int ctxB = *(unsigned int*)(linker + 0x100);

		int idx = -1;
		for (int i = 0; i < count; i++) {
			if (exports[i].NameIndex == objName.Index && exports[i].NameNumber == 0) {
				idx = i;
				break;
			}
		}
		if (idx < 0) {
			Logger::Log("soi", "[SOI] %s: NOT IN EXPORT MAP (%d exports) — asset "
				"not cooked into this package\n", logPath, count);
			return nullptr;
		}

		if (exports[idx].Object) return exports[idx].Object;  // already created

		// Patch load-context flags up the outer chain so the gate passes.
		int walk = idx;
		for (int guard = 0; guard < 16; guard++) {
			exports[walk].FlagsA |= ctxA;
			exports[walk].FlagsB |= ctxB;
			if (exports[walk].OuterIndex <= 0) break;
			walk = exports[walk].OuterIndex - 1;
			if (walk < 0 || walk >= count) break;
		}

		kBeginLoad();
		void* obj = (void*)kCreateExport(linker, idx);
		kEndLoad();  // runs the deferred serialization (property data)

		Logger::Log("soi", "[SOI] %s: forced export #%d -> %p\n", logPath, idx, obj);
		return obj;
	}

	// Resolve "Pkg[.Group].Object" to a live UObject whose class name contains
	// classSubstr. Group-agnostic (short-name find in the global name hash),
	// load-context-proof (forced export creation when the normal load skips).
	UObject* ResolveAssetByPath(const std::string& resPath, const char* classSubstr) {
		const size_t firstDot = resPath.find('.');
		if (firstDot == std::string::npos || firstDot == 0) return nullptr;
		const std::string pkg = resPath.substr(0, firstDot);
		const std::string shortName = resPath.substr(resPath.rfind('.') + 1);
		if (shortName.empty()) return nullptr;

		// File resolvability (cached): guards the LoadPackage assert.
		auto fit = s_pkgFileOk.find(pkg);
		if (fit == s_pkgFileOk.end()) {
			std::string resolved;
			bool ok = FindPackageFileW(Widen(pkg).c_str(), &resolved);
			Logger::Log("soi", "[SOI] FindPackageFile(%s) -> %s%s%s\n", pkg.c_str(),
				ok ? "FOUND" : "NOT FOUND", ok ? " " : "", resolved.c_str());
			fit = s_pkgFileOk.emplace(pkg, ok).first;
		}
		if (!fit->second) return nullptr;

		// Fast path once the package has been loaded this process.
		if (s_pkgLoadedOnce.count(pkg)) {
			FName objName = EngineName(shortName);
			void* hit = kStaticFindObjectFast(nullptr, nullptr, objName.Index, 0, 0, 1, 0, 0x400);
			if (hit && ObjectClassCache::ClassNameContains((UObject*)hit, classSubstr)) {
				return (UObject*)hit;
			}
		}

		// (Re)load the package — cheap when the linker is already live.
		void* pkgRet = kLoadPackage(nullptr, Widen(pkg).c_str(), 0);
		if (!s_pkgLoadedOnce.count(pkg)) {
			Logger::Log("soi", "[SOI] LoadPackage(%s) -> %p\n", pkg.c_str(), pkgRet);
		}
		s_pkgLoadedOnce.insert(pkg);

		FName objName = EngineName(shortName);
		void* hit = kStaticFindObjectFast(nullptr, nullptr, objName.Index, 0, 0, 1, 0, 0x400);
		if (!hit) {
			// Normal load skipped the export (client-only load-context flags)
			// — create it surgically.
			hit = ForceCreateExportByName(pkg, objName, resPath.c_str());
		}
		if (hit && !ObjectClassCache::ClassNameContains((UObject*)hit, classSubstr)) {
			const std::string clsName(ObjectClassCache::GetClassName((UObject*)hit));
			Logger::Log("soi", "[SOI] %s: resolved object has class %s (wanted *%s*) — rejecting\n",
				resPath.c_str(), clsName.c_str(), classSubstr);
			hit = nullptr;
		}
		if (hit) {
			const char* raw = ((UObject*)hit)->GetFullName();  // shared buffer —
			const std::string full(raw ? raw : "<null>");      // copy immediately
			Logger::Log("soi", "[SOI] resolved %s -> %p (%s)\n",
				resPath.c_str(), hit, full.c_str());
		}
		return (UObject*)hit;
	}

	// ── One-shot record: the five retail SOI assets are NOT in the client
	//    cook (user-verified in UE Explorer for ANIMTRE_IceRaptor; the export
	//    map scan below documents the rest). Kept as a one-shot diagnostic so
	//    the absence is in every capture. ──────────────────────────────────
	const char* const kRetailSoiAssets[] = {
		"ANIMTRE_IceRaptor.AT_IceRaptor_SocketOffsetInfo",
		"ANIMTRE_Robot_GuardianBot.AT_Robot_GuardianBot_SocketOffsetInfo",
		"DEV_RocketTurretB.RocketTurretB_SocketOffsetInfo",
		"ROBOT_Boss_uberwalkerA.UberwalkerA_SocketOffsetInfo",
		"ANIMTRE_IceRaptor.AT_Destroyer_SocketOffsetInfo",
	};

	void DumpRetailAssetsOnce() {
		static bool s_dumped = false;
		if (s_dumped || !Logger::IsChannelEnabled("soi")) return;
		s_dumped = true;
		for (const char* path : kRetailSoiAssets) {
			UObject* obj = ResolveAssetByPath(path, "TgSocketOffsetInfo");
			Logger::Log("soi", "[SOI retail-asset] %s -> %p\n", path, (void*)obj);
		}
	}

	// ── SOI synthesis from skeletal-mesh ref-pose sockets ───────────────────
	// The retail SOI data is unrecoverable (server-cook only), but the client
	// meshes ship WITH their sockets. Ref-pose socket position in mesh space:
	//   boneMeshMatrix = Inverse(RefBasesInvMatrix[boneIdx])
	//   pos = TransformPosition(boneMeshMatrix, socket->RelativeLocation)
	// This is the same data the client's GetSocketWorldLocationAndRotation
	// renders from (ref pose, no anim), expressed in the frame the trace
	// native expects (unscaled mesh space, feet at origin; the native applies
	// m_fMeshScale + pawn rotation + feet anchor itself).

	struct M44 { float m[4][4]; };  // UE3 FMatrix: row-major, row-vector convention

	FVector TransformPosition(const M44& M, const FVector& v) {
		FVector out;
		out.X = v.X * M.m[0][0] + v.Y * M.m[1][0] + v.Z * M.m[2][0] + M.m[3][0];
		out.Y = v.X * M.m[0][1] + v.Y * M.m[1][1] + v.Z * M.m[2][1] + M.m[3][1];
		out.Z = v.X * M.m[0][2] + v.Y * M.m[1][2] + v.Z * M.m[2][2] + M.m[3][2];
		return out;
	}

	// Affine inverse for [R 0; t 1] (general 3x3 inverse + translation).
	bool AffineInverse(const M44& M, M44* out) {
		const float a = M.m[0][0], b = M.m[0][1], c = M.m[0][2];
		const float d = M.m[1][0], e = M.m[1][1], f = M.m[1][2];
		const float g = M.m[2][0], h = M.m[2][1], i = M.m[2][2];
		const float A =  (e * i - f * h);
		const float B = -(d * i - f * g);
		const float C =  (d * h - e * g);
		const float det = a * A + b * B + c * C;
		if (det > -1e-8f && det < 1e-8f) return false;
		const float r = 1.0f / det;
		std::memset(out, 0, sizeof(*out));
		out->m[0][0] = A * r;
		out->m[1][0] = B * r;
		out->m[2][0] = C * r;
		out->m[0][1] = -(b * i - c * h) * r;
		out->m[1][1] =  (a * i - c * g) * r;
		out->m[2][1] = -(a * h - b * g) * r;
		out->m[0][2] =  (b * f - c * e) * r;
		out->m[1][2] = -(a * f - c * d) * r;
		out->m[2][2] =  (a * e - b * d) * r;
		// t' = -t * R^-1
		const float tx = M.m[3][0], ty = M.m[3][1], tz = M.m[3][2];
		out->m[3][0] = -(tx * out->m[0][0] + ty * out->m[1][0] + tz * out->m[2][0]);
		out->m[3][1] = -(tx * out->m[0][1] + ty * out->m[1][1] + tz * out->m[2][1]);
		out->m[3][2] = -(tx * out->m[0][2] + ty * out->m[1][2] + tz * out->m[2][2]);
		out->m[3][3] = 1.0f;
		return true;
	}

	// FMeshBone is a native struct the SDK can't type (RefSkeleton shows as
	// TArray<int>). We only need each bone's Name (FName expected at entry
	// offset 0) to map BoneName -> index; the stride is detected empirically:
	// ascending dword-aligned candidates, first one where EVERY entry starts
	// with a sane GNames FName wins. (Ascending matters: candidates smaller
	// than the real stride never read past the buffer, and the real stride
	// passes before any multiple of it.) This build's VJointPos may carry the
	// UE2-style Length+Size tail (stride 0x44), so scan 0x20..0x80.
	bool DetectBoneNameLayout(const char* data, int count, int* outStride, int* outNameOff) {
		const int gnames = FName::Names()->Count;
		for (int stride = 0x20; stride <= 0x80; stride += 4) {
			for (int off = 0; off + 8 <= stride; off += 4) {
				bool ok = true;
				std::unordered_set<unsigned long long> seen;
				for (int i = 0; i < count; i++) {
					const int idx = *(const int*)(data + i * stride + off);
					const int num = *(const int*)(data + i * stride + off + 4);
					if (idx <= 0 || idx >= gnames || num < 0 || num > 0xFFFF) { ok = false; break; }
					FNameEntry* entry = FName::Names()->Data[idx];
					if (!entry || entry->Name[0] == '\0') { ok = false; break; }
					// Bone names are UNIQUE within a skeleton; repeated values
					// mean we latched onto ParentIndex/NumChildren/Depth
					// columns (small ints that also pass the GNames test —
					// exactly the playtest-#6 false positive).
					if (!seen.insert(((unsigned long long)(unsigned int)idx << 32) |
					                 (unsigned int)num).second) { ok = false; break; }
				}
				if (ok) {
					*outStride = stride;
					*outNameOff = off;
					return true;
				}
			}
		}
		// Hand-derivation safety net: dump the head of the array so the
		// layout can be read straight out of the capture.
		if (Logger::IsChannelEnabled("soi")) {
			for (int row = 0; row < 6; row++) {
				const unsigned int* p = (const unsigned int*)(data + row * 0x20);
				Logger::Log("soi",
					"  [bone raw +0x%03X] %08X %08X %08X %08X  %08X %08X %08X %08X\n",
					row * 0x20, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
			}
		}
		return false;
	}

	UTgSocketOffsetInfo* SynthesizeSoiFromMesh(USkeletalMesh* mesh, const std::string& label,
	                                           float zLiftMeshUnits) {
		UClass* cls = SocketOffsetInfoClass();
		if (!cls) return nullptr;

		const int nBones = mesh->RefBasesInvMatrix.Count;
		const int nSkel = mesh->RefSkeleton.Count;
		const char* skel = (const char*)mesh->RefSkeleton.Data;
		if (nBones <= 0 || nBones != nSkel || !skel) {
			Logger::Log("soi", "[SOI synth] %s: skeleton mismatch (invMx=%d refSkel=%d)\n",
				label.c_str(), nBones, nSkel);
			return nullptr;
		}
		if (mesh->Sockets.Count <= 0) {
			Logger::Log("soi", "[SOI synth] %s: mesh has no sockets\n", label.c_str());
			return nullptr;
		}

		int stride = 0, nameOff = 0;
		if (!DetectBoneNameLayout(skel, nBones, &stride, &nameOff)) {
			Logger::Log("soi", "[SOI synth] %s: FMeshBone layout detection FAILED (%d bones)\n",
				label.c_str(), nBones);
			return nullptr;
		}

		// Key by full FName identity (Index + Number) — the engine splits
		// trailing digits into the Number field, so sibling bones like
		// "gun_01"/"gun_02" share an Index and differ only in Number.
		std::unordered_map<unsigned long long, int> boneByName;
		for (int i = 0; i < nBones; i++) {
			const unsigned int idx = *(const unsigned int*)(skel + i * stride + nameOff);
			const unsigned int num = *(const unsigned int*)(skel + i * stride + nameOff + 4);
			boneByName[((unsigned long long)idx << 32) | num] = i;
		}

		FName objName = EngineName(std::string("SOI_Synth_") + label);
		UTgSocketOffsetInfo* soi = (UTgSocketOffsetInfo*)kStaticConstructObject(
			cls, kGetTransientPackage(), objName.Index, 0, 0, 0,
			nullptr, *kGError, 0, nullptr);
		if (!soi) {
			Logger::Log("soi", "[SOI synth] %s: StaticConstructObject failed\n", label.c_str());
			return nullptr;
		}
		// Rooted: cached in s_synthByMeshName with nothing else referencing it
		// between spawns — GC must never reclaim it.
		soi->ObjectFlags.A |= 0x4000;  // RF_RootSet (established idiom)

		const M44* invMx = (const M44*)mesh->RefBasesInvMatrix.Data;
		Logger::Log("soi", "[SOI synth] %s: %d sockets, %d bones, stride=0x%X nameOff=0x%X zLift=%.1f\n",
			label.c_str(), mesh->Sockets.Count, nBones, stride, nameOff, zLiftMeshUnits);

		for (int s = 0; s < mesh->Sockets.Count; s++) {
			USkeletalMeshSocket* sock = mesh->Sockets.Data[s];
			if (!sock) continue;
			const unsigned int bnum = *(const unsigned int*)((const char*)&sock->BoneName + 4);
			auto bit = boneByName.find(((unsigned long long)(unsigned int)sock->BoneName.Index << 32) | bnum);
			if (bit == boneByName.end()) {
				Logger::Log("soi", "  [%d] %s: bone '%s' not in skeleton — skipped\n",
					s, sock->SocketName.GetName(), sock->BoneName.GetName());
				continue;
			}
			M44 boneM;
			if (!AffineInverse(invMx[bit->second], &boneM)) {
				Logger::Log("soi", "  [%d] %s: singular bone matrix — skipped\n",
					s, sock->SocketName.GetName());
				continue;
			}
			FVector meshPos = TransformPosition(boneM, sock->RelativeLocation);

			// Mesh frame -> pawn frame. Meshes are authored +Y-forward /
			// +X-left (playtest #7: every "*Right" bone has negative mesh-X
			// on all dumped meshes; Z=0 at feet per CSO_AoE_Stomp); the trace
			// native consumes pawn-local X-forward / Y-right offsets. 90° yaw:
			FVector pos;
			pos.X = meshPos.Y;
			pos.Y = -meshPos.X;
			pos.Z = meshPos.Z + zLiftMeshUnits;

			FSocketOffsetInfo e;
			std::memset(&e, 0, sizeof(e));
			e.SocketName = sock->SocketName;
			e.AimProfileName = FName(0);
			// No retail aim grid to reconstruct — every aim cell gets the
			// ref-pose position (GetInterpLocation then returns it for any
			// aim direction).
			e.LU = e.LC = e.LD = e.CU = e.CC = e.CD = e.RU = e.RC = e.RD = pos;
			soi->m_SocketOffsets.Add(e);

			Logger::Log("soi", "  [%d] %s @ bone %s mesh=(%.2f, %.2f, %.2f) pawn=(fwd %.2f, right %.2f, up %.2f)\n",
				s, sock->SocketName.GetName(), sock->BoneName.GetName(),
				meshPos.X, meshPos.Y, meshPos.Z, pos.X, pos.Y, pos.Z);
		}

		if (soi->m_SocketOffsets.Count == 0) {
			Logger::Log("soi", "[SOI synth] %s: no usable sockets\n", label.c_str());
			return nullptr;
		}
		return soi;
	}

	// Collect the ShotOrigin socket FName indexes this assembly's FX data
	// expects (direct iteration of the model entry vector, +0x88/+0x8C;
	// entry: +0x8 socket FName, +0x10 group FName).
	bool HasShotOriginSockets(void* model) {
		const int groupIdx = ShotOriginName().Index;
		char** it  = *(char***)((char*)model + 0x88);
		char** end = *(char***)((char*)model + 0x8c);
		if (!it || !end || it > end || (end - it) > 4096) return false;
		for (; it != end; ++it) {
			char* e = *it;
			if (e && ((FName*)(e + 0x10))->Index == groupIdx &&
			    ((FName*)(e + 0x8))->Index > 0) {
				return true;
			}
		}
		return false;
	}

}  // namespace

void* FireSockets::GetMeshModel(int asmId) {
	if (asmId <= 0) return nullptr;
	return kGetMeshModel(kAssemblyManager, (unsigned int)asmId);
}

FName FireSockets::GetShotOriginSocketName(void* model, int fireMode, int socketIndex, int equipPoint) {
	FName out(0);
	if (!model) return out;
	FName group = ShotOriginName();
	kGetSocketName(model, &out, group.Index, 0, fireMode, socketIndex, equipPoint);
	return out;
}

int FireSockets::GetShotOriginSocketMax(void* model, int fireMode, int equipPoint) {
	if (!model) return 1;
	FName group = ShotOriginName();
	return kGetSocketMax(model, group.Index, 0, fireMode, equipPoint);
}

void FireSockets::EnsurePopulated(ATgPawn* Pawn) {
	if (!Pawn || Pawn->m_TgSocketOffsetInfo) return;

	const int asmId = Pawn->r_nBodyMeshAsmId;
	if (asmId <= 0) return;
	if (s_noSoiAsmIds.count(asmId)) return;

	DumpRetailAssetsOnce();

	void* model = GetMeshModel(asmId);
	if (!model) {
		s_noSoiAsmIds.insert(asmId);
		return;
	}

	// Only assemblies whose FX data defines ShotOrigin sockets get a synth
	// SOI; everything else keeps the retail eye-height fallback.
	if (!HasShotOriginSockets(model)) {
		s_noSoiAsmIds.insert(asmId);
		return;
	}

	// The trace native multiplies the SOI offset by m_fMeshScale (+0x1264) —
	// normally set by ApplyPawnSetup, which never runs for server-side bots,
	// leaving it 0 and collapsing every offset to the feet anchor ("rockets
	// from between the feet", playtest #8). Populate it from the asm scale.
	if (Pawn->m_fMeshScale <= 0.0f) {
		const float scale = ModelScale(model);
		Pawn->m_fMeshScale = (scale > 0.0f) ? scale : 1.0f;
	}

	FName meshRes = ModelMeshResName(model);
	if (meshRes.Index <= 0 || meshRes.Index >= FName::Names()->Count) {
		s_noSoiAsmIds.insert(asmId);
		return;
	}

	auto cached = s_synthByAsmId.find(asmId);
	if (cached != s_synthByAsmId.end()) {
		if (cached->second) Pawn->m_TgSocketOffsetInfo = cached->second;
		return;
	}

	// Anchor correction. Ground meshes are feet-authored and the client puts
	// their origin at the cylinder bottom — matching the trace native's feet
	// anchor (proven: bots sink without the spawn-Z lift; Shrike POC ground
	// truth). Hover/flying meshes are body-authored (negative-Z gun sockets,
	// e.g. Desert Guardian) and render around the pawn CENTER with the
	// cylinder hanging underneath — so their offsets need +CollisionHeight,
	// expressed in unscaled mesh units (the native multiplies by m_fMeshScale).
	//
	// Class-driven, NOT Physics==4: PHYS_Flying is applied later by the
	// PostPawnSetup ProcessEvent hook, so it's never set yet when this runs
	// from SpawnBotById. List = flying classes from that hook + TgPawn_Guardian
	// (ground-physics but renders airborne: its PostPawnSetup forces the
	// physics-asset instance at PhysicsWeight=1, so the client draws the body
	// from rigid-body sim above the cylinder — Support Guardian asm 757).
	float zLift = 0.0f;
	bool airborneVisual =
		ObjectClassCache::ClassNameContains(Pawn, "TgPawn_Hover")           ||
		ObjectClassCache::ClassNameContains(Pawn, "TgPawn_FlyingBoss")      ||
		ObjectClassCache::ClassNameContains(Pawn, "TgPawn_AttackTransport") ||
		ObjectClassCache::ClassNameContains(Pawn, "TgPawn_ColonyEye")       ||
		ObjectClassCache::ClassNameContains(Pawn, "TgPawn_NewWasp")         ||
		ObjectClassCache::ClassNameContains(Pawn, "TgPawn_Guardian");
	if (!airborneVisual && Pawn->m_pAmBot.Dummy) {
		// Ground-class pawns mounting a flying mesh (same whitelist as the
		// PostPawnSetup PHYS_Flying hook).
		const int botId = *(int*)((char*)Pawn->m_pAmBot.Dummy + 0x1C);
		airborneVisual = (botId == 1107 || botId == 1657);
	}
	if (airborneVisual) {
		const float collH = Pawn->CylinderComponent
			? Pawn->CylinderComponent->CollisionHeight : 0.0f;
		zLift = collH / Pawn->m_fMeshScale;
	}

	const std::string meshPath(meshRes.GetName());
	USkeletalMesh* mesh = (USkeletalMesh*)ResolveAssetByPath(meshPath, "SkeletalMesh");
	UTgSocketOffsetInfo* soi = nullptr;
	if (mesh) {
		soi = SynthesizeSoiFromMesh(mesh, meshPath, zLift);
	}
	s_synthByAsmId[asmId] = soi;  // negative-cached as nullptr too

	Logger::Log("soi", "[SOI populate] asm=%d mesh=%s -> %p (meshScale=%.2f)\n",
		asmId, meshPath.c_str(), (void*)soi, Pawn->m_fMeshScale);

	if (soi) {
		Pawn->m_TgSocketOffsetInfo = soi;  // UObjectProperty — GC-traced (and rooted)
	} else {
		s_noSoiAsmIds.insert(asmId);
	}
}
