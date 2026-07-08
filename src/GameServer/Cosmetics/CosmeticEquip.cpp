#include "src/pch.hpp"
#include "src/GameServer/Cosmetics/CosmeticEquip.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/Database/Database.hpp"
#include "src/Shared/CosmeticSlots.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace CosmeticEquip {

// Forward declaration — definition further down. Used by LoadFromDB to dump
// the struct after each cosmetic ApplyOnly write.
static void SpawnAsmSnapshot(const char* where, void* obj, const FCustomCharacterAssembly& a);

// Engine equip point → EDyeSlots enum value
// (TgObject.uc:173 — Primary=0, Secondary=1, Emissive=2, WeaponColor=3,
// WeaponEmissive=4). Anything outside the 5 dye slots returns -1.
static int SlotToEDyeSlot(int slot) {
	switch (slot) {
		case 16: return 0;  // SVID_DYE_PRIMARY (996)
		case 17: return 1;  // SVID_DYE_SECONDARY (997)
		case 18: return 2;  // SVID_DYE_EMISSIVE (998)
		case 19: return 3;  // SVID_DYE_WEAPON_PRIMARY (999)
		case 20: return 4;  // SVID_DYE_WEAPON_EMISSIVE (1000)
		default: return -1;
	}
}

// Look up (item_type_value_id, item_subtype_value_id) for a game item_id
// (asm_data_set_items.item_id, NOT the row PK .id). Both zero on miss.
static void LookupItemTypeAndSubtype(int itemId, int& outType, int& outSubtype) {
	outType = 0;
	outSubtype = 0;
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT item_type_value_id, item_subtype_value_id "
	        "FROM asm_data_set_items WHERE item_id = ?",
	        -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, itemId);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			outType    = sqlite3_column_int(stmt, 0);
			outSubtype = sqlite3_column_int(stmt, 1);
		}
		sqlite3_finalize(stmt);
	}
}

// Resolve the body-mesh asm_id for a cosmetic item, picked by gender.
// Returns 0 on miss. Falls back to the male row if the player's gender has no
// dedicated mesh (Commonwealth Technician is male-only in the data).
//
// `genderTypeId` is r_CustomCharacterAssembly.nGenderTypeId from the pawn —
// see GA_G constants for the canonical Male/Female values.
static int LookupMeshAsmIdForCosmetic(int itemId, int genderTypeId) {
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	int asmId = 0;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT asm_id FROM asm_data_set_item_mesh_asm_groups "
	        "WHERE item_id = ? "
	        "ORDER BY CASE gender_type_value_id WHEN ? THEN 0 ELSE 1 END LIMIT 1",
	        -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, itemId);
		sqlite3_bind_int(stmt, 2, genderTypeId);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			asmId = sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}
	return asmId;
}

struct MeshCollisionData {
	float radius;
	float halfHeight;
	float crouchHeight;
	float hitHeight;
	float hitRadius;
};

static const MeshCollisionData& LookupMeshCollisionData(int meshAsmId) {
	static std::unordered_map<int, MeshCollisionData> cache;
	auto it = cache.find(meshAsmId);
	if (it != cache.end()) return it->second;

	MeshCollisionData data{25.0f, 46.0f, 0.0f, 0.0f, 0.0f};
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT collision_radius, collision_height, crouch_height, "
			"       hit_collision_height, hit_collision_radius "
			"FROM asm_data_set_assembly_meshes "
			"WHERE asm_id = ? LIMIT 1",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, meshAsmId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				const float radius     = (float)sqlite3_column_double(stmt, 0);
				const float halfHeight = (float)sqlite3_column_double(stmt, 1);
				const float crouch     = (float)sqlite3_column_double(stmt, 2);
				const float hitHeight  = (float)sqlite3_column_double(stmt, 3);
				const float hitRadius  = (float)sqlite3_column_double(stmt, 4);
				if (radius > 0.0f) data.radius = radius;
				if (halfHeight > 0.0f) data.halfHeight = halfHeight;
				if (crouch > 0.0f) data.crouchHeight = crouch;
				if (hitHeight > 0.0f) data.hitHeight = hitHeight;
				if (hitRadius > 0.0f) data.hitRadius = hitRadius;
			}
			sqlite3_finalize(stmt);
		}
	}

	auto [inserted, _] = cache.emplace(meshAsmId, data);
	return inserted->second;
}

static void ApplyMeshCollisionData(ATgPawn* Pawn, int meshAsmId) {
	if (!Pawn || meshAsmId <= 0) return;
	const MeshCollisionData& data = LookupMeshCollisionData(meshAsmId);

	if (data.radius > 0.0f && data.halfHeight > 0.0f) {
		Pawn->SetCollisionSize(data.radius, data.halfHeight);
	}
	if (data.crouchHeight > 0.0f) Pawn->CrouchHeight = data.crouchHeight;
	if (data.radius > 0.0f) Pawn->CrouchRadius = data.radius;
	if (data.hitHeight > 0.0f) Pawn->m_fTargetCylinderHeight = data.hitHeight;
	if (data.hitRadius > 0.0f) Pawn->m_fTargetCylinderRadius = data.hitRadius;

	Pawn->m_fStandingHeight = data.halfHeight;
	Pawn->m_fStandingRadius = data.radius;
	Logger::Log("cosmetic-equip",
		"LoadFromDB: mesh collision asm=%d radius=%.2f halfHeight=%.2f\n",
		meshAsmId, data.radius, data.halfHeight);
}

static constexpr int kNoCosmeticItemId = 0;

// Per-class, per-gender default body suit shown when no cosmetic suit is
// equipped. Found via asm_data_set_assembly_meshes — male rows are named
// "*_SUIT_DEFAULT" (asm 1354-1357); female rows are separate, mislabeled
// rows named "Female Suit <Class> DEFAULT" (asm 1482-1485) that an earlier
// case-sensitive `LIKE '%SUIT%DEFAULT%'` search missed entirely (DuckDB LIKE
// is case-sensitive; "Suit" != "SUIT"). Not a catalog item — no
// LookupMeshAsmIdForCosmetic lookup applies, these are raw asm_ids.
static int GetClassDefaultSuitAsmId(uint32_t profileId, bool isFemale) {
	if (isFemale) {
		switch (profileId) {
			case 567: return 1484;  // Female Suit Medic DEFAULT
			case 679: return 1482;  // Female Suit Robotics DEFAULT
			case 680: return 1485;  // Female Suit Assault DEFAULT
			case 681: return 1483;  // Female Suit Recon DEFAULT
			default:  return 1485;
		}
	}
	switch (profileId) {
		case 567: return 1355;  // MEDIC_SUIT_DEFAULT
		case 679: return 1357;  // ROBOTICS_SUIT_DEFAULT
		case 680: return 1354;  // ASSAULT_SUIT_DEFAULT
		case 681: return 1356;  // RECON_SUIT_DEFAULT
		default:  return 1354;
	}
}

static void ApplyClassVisualFallback(ATgPawn* Pawn, uint32_t profileId) {
	if (!Pawn) return;
	auto* charPawn = (ATgPawn_Character*)Pawn;
	auto* PRI      = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	if (charPawn->r_CustomCharacterAssembly.SuitMeshId <= 0) {
		const bool isFemale = charPawn->r_CustomCharacterAssembly.nGenderTypeId == GA_G::GENDER_TYPE_ID_FEMALE;
		const int meshAsm = GetClassDefaultSuitAsmId(profileId, isFemale);
		charPawn->r_CustomCharacterAssembly.SuitMeshId = meshAsm;
		if (PRI) PRI->r_CustomCharacterAssembly.SuitMeshId = meshAsm;
	}

	if (charPawn->r_CustomCharacterAssembly.HeadFlairId <= 0) {
		charPawn->r_CustomCharacterAssembly.HelmetMeshId = -1;
		if (PRI) PRI->r_CustomCharacterAssembly.HelmetMeshId = -1;
	}
}

// Core pawn-state write. Doesn't touch the DB.
// Used by both ApplyToPawn (which then persists) and LoadFromDB (which
// reads from DB and replays without re-persisting).
//
// `slot` is the DB storage slot — for cosmetic suit/helmet that's the
// CosmeticSlots-remapped value (22/23), for dyes/trail it's the engine slot.
// Only the dye path uses `slot` directly (to index into DyeList[]); the
// helmet/suit paths write a single struct field and ignore `slot`.
//
// Mirrors every assembly field write to the pawn's PRI
// (ATgRepInfo_Player::r_CustomCharacterAssembly at offset 0x3EC, also
// CPF_Net). The PRI's copy drives UpdateCharacterAssetRefs() in
// TgRepInfo_Player.uc:303 which preloads the mesh/material/particle assets
// the cosmetic refers to. Without the PRI mirror, the client preloads the
// "no flair" asset set (initial spawn values: SuitFlairId=-1, HeadFlairId=-1,
// JetpackTrailId=0, DyeList=0), then later receives the pawn's
// assembly diff which references unpreloaded assets and triggers an on-demand
// load whose timing window is what causes the Mace and Shield right-click
// shield to bind to stale skeleton bones.
static bool ApplyOnly(ATgPawn* Pawn, int slot, int itemId) {
	if (!Pawn) return false;
	auto* charPawn = (ATgPawn_Character*)Pawn;
	auto* PRI      = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	auto markDirty = [&]() {
		Pawn->bNetDirty       = 1;
		Pawn->bForceNetUpdate = 1;
		if (PRI) {
			PRI->bNetDirty       = 1;
			PRI->bForceNetUpdate = 1;
		}
	};

	int itype = 0, isub = 0;
	LookupItemTypeAndSubtype(itemId, itype, isub);
	Logger::Log("spawn-asm",
		"ApplyOnly: slot=%d itemId=%d -> itype=%d isub=%d\n",
		slot, itemId, itype, isub);

	if (isub == 1006 || isub == 1007) {
		// Helmet (1006 base) OR Helmet Flair (1007) → HeadFlairId.
		// Per the flair-priority rule in
		// TgSkeletalMeshActor_CharacterBuilder.uc:360, HeadFlairId overrides
		// HelmetMeshId visually.
		charPawn->r_CustomCharacterAssembly.HeadFlairId = itemId;
		if (PRI) {
			PRI->r_CustomCharacterAssembly.HeadFlairId = itemId;
		}
		// Also mirror to HelmetMeshId. The flair path picks the visible mesh
		// from the flair, but other engine systems may consult HelmetMeshId
		// directly; keeping them aligned avoids unnecessary asset divergence.
		const int gender = charPawn->r_CustomCharacterAssembly.nGenderTypeId;
		const int meshAsm = LookupMeshAsmIdForCosmetic(itemId, gender);
		if (meshAsm > 0) {
			charPawn->r_CustomCharacterAssembly.HelmetMeshId = meshAsm;
			if (PRI) PRI->r_CustomCharacterAssembly.HelmetMeshId = meshAsm;
		}
		markDirty();
		Logger::Log("cosmetic-equip",
			"ApplyOnly: HeadFlairId=%d HelmetMeshId=%d (subtype %d) slot %d (pawn+PRI)\n",
			itemId, meshAsm, isub, slot);
		return true;
	}

	if (isub == 1008) {
		charPawn->r_CustomCharacterAssembly.SuitFlairId = itemId;
		if (PRI) {
			PRI->r_CustomCharacterAssembly.SuitFlairId = itemId;
		}
		// CRITICAL: also write SuitMeshId. TgSkeletalMeshActor_CharacterBuilder
		// .uc:237 keys m_bAreAnimationsDirty off m_Assembly.SuitMeshId — the
		// animation set is derived from SuitMeshId, NOT SuitFlairId. If
		// SuitMeshId is left at the spawn-time placeholder (1225) while
		// SuitFlairId points to a cosmetic with a different skeleton (e.g.
		// "Robotics Commonwealth Technician" → asm_id 1586), the rendered
		// body mesh uses the cosmetic's skeleton but plays animations cut
		// for 1225's skeleton. Bones end up in the wrong rest pose →
		// dual-wield melee secondary attachments bind to stale transforms.
		// Symptom: Mace and Shield's right-click shield visual rotated
		// ~90° CCW on first spawn with the cosmetic, "fixed" only by
		// swapping suits + melee (which re-runs ApplyPawnSetup).
		const int gender = charPawn->r_CustomCharacterAssembly.nGenderTypeId;
		const int meshAsm = LookupMeshAsmIdForCosmetic(itemId, gender);
		if (meshAsm > 0) {
			charPawn->r_CustomCharacterAssembly.SuitMeshId = meshAsm;
			if (PRI) PRI->r_CustomCharacterAssembly.SuitMeshId = meshAsm;
		}
		markDirty();
		Logger::Log("cosmetic-equip",
			"ApplyOnly: SuitFlairId=%d SuitMeshId=%d slot %d (pawn+PRI)\n",
			itemId, meshAsm, slot);
		return true;
	}

	if (itype == 1020) {
		const int eSlot = SlotToEDyeSlot(slot);
		if (eSlot < 0) {
			Logger::Log("cosmetic-equip",
				"ApplyOnly: dye %d on non-dye slot %d — skipped\n",
				itemId, slot);
			return false;
		}
		// Direct field write. r_CustomCharacterAssembly is CPF_Net; the
		// replicated diff triggers the client's own ApplyDye via the
		// ReplicatedEvent handler. Calling ApplyDye on the server iterates
		// the pawn's MICs / mesh components and was confirmed to side-effect
		// dual-wield melee weapon attachments (Mace and Shield's left-hand
		// shield was rotating 90° CCW after spawn-time apply).
		charPawn->r_CustomCharacterAssembly.DyeList[eSlot] = itemId;
		if (PRI) {
			PRI->r_CustomCharacterAssembly.DyeList[eSlot] = itemId;
		}
		markDirty();
		Logger::Log("cosmetic-equip",
			"ApplyOnly: DyeList[%d]=%d slot %d (pawn+PRI, direct write)\n",
			eSlot, itemId, slot);
		return true;
	}

	if (itype == 1612) {
		// Same direct-write rationale as the dye path above. The client's
		// ReplicatedEvent handler will refresh the particle component when
		// the assembly diff arrives.
		charPawn->r_CustomCharacterAssembly.JetpackTrailId = itemId;
		if (PRI) {
			PRI->r_CustomCharacterAssembly.JetpackTrailId = itemId;
		}
		markDirty();
		Logger::Log("cosmetic-equip",
			"ApplyOnly: JetpackTrailId=%d slot %d (pawn+PRI)\n",
			itemId, slot);
		return true;
	}

	Logger::Log("cosmetic-equip",
		"ApplyOnly: unrecognized item type=%d subtype=%d for itemId %d\n",
		itype, isub, itemId);
	Logger::Log("spawn-asm",
		"ApplyOnly: UNRECOGNIZED itype=%d isub=%d itemId=%d slot=%d (no write)\n",
		itype, isub, itemId, slot);
	return false;
}

// One-line struct snapshot for diagnostic timeline. Defined here so it lives
// in the same TU as ApplyOnly; SpawnPlayerCharacter.cpp has its own copy with
// the same format.
static void SpawnAsmSnapshot(const char* where, void* obj, const FCustomCharacterAssembly& a) {
	Logger::Log("spawn-asm",
		"%s obj=%p Suit=%d Head=%d Hair=%d Helmet=%d Skin=%d SkinRace=%d Eye=%d "
		"bBald=%d bHideHelmet=%d bValid=%d bHalfHelmet=%d Gender=%d "
		"HeadFlair=%d SuitFlair=%d Trail=%d Dye=[%d,%d,%d,%d,%d]\n",
		where, obj,
		a.SuitMeshId, a.HeadMeshId, a.HairMeshId, a.HelmetMeshId,
		a.SkinToneParameterId, a.SkinRaceParameterId, a.EyeColorParameterId,
		(int)a.bBald, (int)a.bHideHelmet, (int)a.bValidCustomAssembly, (int)a.bHalfHelmet,
		a.nGenderTypeId,
		a.HeadFlairId, a.SuitFlairId, a.JetpackTrailId,
		a.DyeList[0], a.DyeList[1], a.DyeList[2], a.DyeList[3], a.DyeList[4]);
}

static int NormalizeItemProfileId(ATgPawn* Pawn, int item_profile_id) {
	if (item_profile_id >= 1 && item_profile_id <= 5) return item_profile_id;
	if (Pawn) {
		auto* charPawn = (ATgPawn_Character*)Pawn;
		if (charPawn->r_nItemProfileId >= 1 && charPawn->r_nItemProfileId <= 5) {
			return charPawn->r_nItemProfileId;
		}
	}
	return 1;
}

bool ApplyToPawn(ATgPawn* Pawn, int64_t character_id, int item_profile_id,
                 int slot, int invId, int itemId) {
	item_profile_id = NormalizeItemProfileId(Pawn, item_profile_id);

	// Resolve subtype up front so we can both pick the DB slot AND have it
	// available when ApplyOnly looks itself up. Cheap second query duplicates
	// what ApplyOnly does internally; keeping ApplyOnly self-contained matters
	// more here than shaving one prepare.
	int itype = 0, isub = 0;
	LookupItemTypeAndSubtype(itemId, itype, isub);

	// `slot` arriving from the client is the engine equip point (e.g. 6 for
	// suit, 12 for helmet). For cosmetic suit/helmet we MUST persist at the
	// remapped DB slot, otherwise cosmetic suit/helmet rows collide with
	// gameplay suit/helmet rows in the same profile.
	const int dbSlot = CosmeticSlots::DbSlotFor(slot, isub);

	if (!ApplyOnly(Pawn, dbSlot, itemId)) return false;

	// Persist profile-scoped cosmetics. Suit/helmet use DB slots 22/23;
	// dyes and trail keep their engine slot.
	sqlite3* db = Database::GetConnection();
	if (!db) return false;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
	        "INSERT OR REPLACE INTO ga_character_devices "
	        "  (character_id, item_profile_id, inventory_id, equipped_slot) VALUES (?, ?, ?, ?)",
	        -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(stmt, 1, character_id);
		sqlite3_bind_int  (stmt, 2, item_profile_id);
		sqlite3_bind_int  (stmt, 3, invId);
		sqlite3_bind_int  (stmt, 4, dbSlot);
		const int stepResult = sqlite3_step(stmt);
		if (stepResult != SQLITE_DONE) {
			Logger::Log("cosmetic-equip",
				"ApplyToPawn: persist FAILED char=%lld itemProf=%d dbSlot=%d invId=%d: %s\n",
				(long long)character_id, item_profile_id, dbSlot, invId, sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return false;
		}
		sqlite3_finalize(stmt);
	} else {
		Logger::Log("cosmetic-equip", "ApplyToPawn: prepare(insert) failed: %s\n",
			sqlite3_errmsg(db));
		return false;
	}
	Logger::Log("cosmetic-equip",
		"ApplyToPawn: char=%lld itemProf=%d engineSlot=%d dbSlot=%d invId=%d itemId=%d (type=%d sub=%d) persisted\n",
		(long long)character_id, item_profile_id, slot, dbSlot, invId, itemId, itype, isub);
	return true;
}

bool ClearSlot(ATgPawn* Pawn, int64_t character_id, int item_profile_id, int slot) {
	if (!Pawn) return false;
	item_profile_id = NormalizeItemProfileId(Pawn, item_profile_id);
	if (slot != 6 && slot != 12 && slot != 21 && (slot < 16 || slot > 20)) return false;

	auto* charPawn = (ATgPawn_Character*)Pawn;
	auto* PRI      = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	// Engine slot → DB storage slot. Suit (6) / Helmet (12) use the
	// CosmeticSlots remap (22/23); dyes pass through unchanged.
	int dbSlot = slot;
	if (slot == 6) {
		charPawn->r_CustomCharacterAssembly.SuitFlairId = -1;
		charPawn->r_CustomCharacterAssembly.SuitMeshId  = -1;
		if (PRI) {
			PRI->r_CustomCharacterAssembly.SuitFlairId = -1;
			PRI->r_CustomCharacterAssembly.SuitMeshId  = -1;
		}
		dbSlot = CosmeticSlots::kCosmeticSuitDbSlot;
	} else if (slot == 12) {
		charPawn->r_CustomCharacterAssembly.HeadFlairId  = -1;
		charPawn->r_CustomCharacterAssembly.HelmetMeshId = -1;
		if (PRI) {
			PRI->r_CustomCharacterAssembly.HeadFlairId  = -1;
			PRI->r_CustomCharacterAssembly.HelmetMeshId = -1;
		}
		dbSlot = CosmeticSlots::kCosmeticHelmetDbSlot;
	} else if (slot == 21) {
		charPawn->r_CustomCharacterAssembly.JetpackTrailId = kNoCosmeticItemId;
		if (PRI) PRI->r_CustomCharacterAssembly.JetpackTrailId = kNoCosmeticItemId;
	} else {
		charPawn->r_CustomCharacterAssembly.DyeList[slot - 16] = kNoCosmeticItemId;
		if (PRI) PRI->r_CustomCharacterAssembly.DyeList[slot - 16] = kNoCosmeticItemId;
	}

	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
		        "DELETE FROM ga_character_devices "
		        "WHERE character_id = ? AND item_profile_id = ? AND equipped_slot = ?",
		        -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(stmt, 1, character_id);
			sqlite3_bind_int  (stmt, 2, item_profile_id);
			sqlite3_bind_int  (stmt, 3, dbSlot);
			if (sqlite3_step(stmt) != SQLITE_DONE) {
				Logger::Log("cosmetic-equip", "ClearSlot: DELETE FAILED char=%lld dbSlot=%d: %s\n",
					(long long)character_id, dbSlot, sqlite3_errmsg(db));
			}
			sqlite3_finalize(stmt);
		} else {
			Logger::Log("cosmetic-equip", "ClearSlot: prepare(delete) failed: %s\n",
				sqlite3_errmsg(db));
		}
	}

	ApplyClassVisualFallback(Pawn, charPawn->r_nProfileId);
	// 1225 is a collision-lookup key only when no suit is equipped — see the
	// matching comment in LoadFromDB; must not be written back to SuitMeshId.
	int collisionLookupMesh = charPawn->r_CustomCharacterAssembly.SuitMeshId;
	if (collisionLookupMesh <= 0) collisionLookupMesh = 1225;
	ApplyMeshCollisionData(Pawn, collisionLookupMesh);
	if (PRI) PRI->r_CustomCharacterAssembly = charPawn->r_CustomCharacterAssembly;
	Pawn->bNetDirty       = 1;
	Pawn->bForceNetUpdate = 1;
	if (PRI) {
		PRI->bNetDirty       = 1;
		PRI->bForceNetUpdate = 1;
	}

	Logger::Log("cosmetic-equip",
		"ClearSlot: char=%lld itemProf=%d engineSlot=%d dbSlot=%d\n",
		(long long)character_id, item_profile_id, slot, dbSlot);
	return true;
}

struct CharacterBaseState {
	int headMesh = 1605;
	int genderId = GA_G::GENDER_TYPE_ID_MALE;
	uint32_t profileId = 680;
	int hairMesh = 1757;
};

// Reads the per-character creation fields from ga_characters and decodes the
// morph blob into the pawn's replicated morph arrays. Returns the base state
// (head mesh, gender, profile, hair mesh) with defaults if the row is missing.
static CharacterBaseState ReadCharacterBaseState(int64_t character_id, ATgPawn_Character* charPawn) {
	CharacterBaseState state;
	sqlite3* db = Database::GetConnection();
	if (!db) return state;

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
		"SELECT head_asm_id, gender_type_value_id, profile_id, morph_data, hair_asm_id "
		"FROM ga_characters WHERE id = ?",
		-1, &stmt, nullptr) != SQLITE_OK) return state;

	enum Col { head_asm_id = 0, gender_type_value_id, profile_id, morph_data, hair_asm_id };
	sqlite3_bind_int64(stmt, 1, character_id);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		const int h = sqlite3_column_int(stmt, Col::head_asm_id);
		const int g = sqlite3_column_int(stmt, Col::gender_type_value_id);
		const int p = sqlite3_column_int(stmt, Col::profile_id);
		const int hr = sqlite3_column_int(stmt, Col::hair_asm_id);
		if (h > 0) state.headMesh = h;
		if (g > 0) state.genderId = g;
		if (p > 0) state.profileId = (uint32_t)p;
		if (hr > 0) state.hairMesh = hr;

		struct MorphPair { uint32_t index; uint32_t weight; };
		static_assert(sizeof(MorphPair) == 8, "morph blob stride");

		const auto* blob = (const uint8_t*)sqlite3_column_blob(stmt, Col::morph_data);
		const int pairCount = sqlite3_column_bytes(stmt, Col::morph_data) / (int)sizeof(MorphPair);
		int maxIdx = -1;
		for (int i = 0; i < pairCount; ++i) {
			MorphPair pair;
			memcpy(&pair, blob + i * sizeof(MorphPair), sizeof(MorphPair));
			if (pair.index < 255u) {
				charPawn->r_nMorphSettings[pair.index] = (int)pair.weight;
				if ((int)pair.index > maxIdx) maxIdx = (int)pair.index;
			}
		}
		if (maxIdx >= 0) {
			charPawn->r_nMaxMorphIndexSentFromServer = maxIdx + 1;
			Logger::Log("spawn-asm",
				"LoadFromDB morph: char=%lld decoded %d pairs (maxIdx=%d)\n",
				(long long)character_id, pairCount, maxIdx);
		}
	}
	sqlite3_finalize(stmt);
	return state;
}

// Baseline assembly used as the starting point before overlaying DB cosmetics.
// Mirrors what SpawnPlayerCharacter used to write inline, except SuitMeshId
// starts at -1 instead of the 1225 Medic Acolyte placeholder — the cosmetic
// overlay pass below fills it from the actual suit's mesh asm, and the
// fallback for characters with no cosmetic suit is applied at the end.
//
// `headMesh`, `hairMesh`, `genderId` come from ga_characters (per-character
// state set at character creation). Pre-cosmetic-system code hardcoded these
// to a Jungle Troll head + male defaults, which then leaked through any
// head/helmet flair cosmetic that overlays on top of the base head (the flair
// system doesn't replace the underlying face/scalp). Reading them from the DB
// gives the player the head + gender they actually picked.
static void WriteBaselineAssembly(FCustomCharacterAssembly& a,
                                  int headMesh, int hairMesh, int genderId) {
	a.SuitMeshId            = -1;
	a.HeadMeshId            = headMesh;
	a.HairMeshId            = hairMesh;
	a.HelmetMeshId          = -1;
	a.SkinToneParameterId   = 0;
	a.SkinRaceParameterId   = 0;
	a.EyeColorParameterId   = 0;
	a.bBald                 = false;
	a.bHideHelmet           = false;
	a.bValidCustomAssembly  = true;
	a.bHalfHelmet           = false;
	a.nGenderTypeId         = genderId;
	a.HeadFlairId           = -1;
	a.SuitFlairId           = -1;
	a.JetpackTrailId        = kNoCosmeticItemId;
	a.DyeList[0]            = kNoCosmeticItemId;
	a.DyeList[1]            = kNoCosmeticItemId;
	a.DyeList[2]            = kNoCosmeticItemId;
	a.DyeList[3]            = kNoCosmeticItemId;
	a.DyeList[4]            = kNoCosmeticItemId;
}

void LoadFromDB(ATgPawn* Pawn, int64_t character_id, int item_profile_id) {
	if (!Pawn) return;
	auto* charPawn = (ATgPawn_Character*)Pawn;
	auto* PRI      = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	item_profile_id = NormalizeItemProfileId(Pawn, item_profile_id);

	// (0) Read per-character creation state (head, hair, gender, morph poses)
	// from ga_characters and write morph arrays onto the pawn.
	const CharacterBaseState base = ReadCharacterBaseState(character_id, charPawn);
	const int headMesh = base.headMesh;
	const int genderId = base.genderId;
	const uint32_t profileId = base.profileId;
	const int hairMesh = base.hairMesh;

	// (1) Initialize the baseline assembly. Owning this here (instead of in
	// SpawnPlayerCharacter) keeps the cosmetic state machine in one place and
	// removes the placeholder-then-overwrite dance that left r_nBodyMeshAsmId
	// pointing at the Medic Acolyte skeleton (1225) while SuitMeshId pointed
	// at the cosmetic skeleton (e.g. 1586 for Commonwealth Technician).
	Logger::Log("spawn-asm",
		"LoadFromDB baseline: char=%lld itemProf=%d head=%d hair=%d gender=%d\n",
		(long long)character_id, item_profile_id, headMesh, hairMesh, genderId);
	WriteBaselineAssembly(charPawn->r_CustomCharacterAssembly, headMesh, hairMesh, genderId);
	if (PRI) WriteBaselineAssembly(PRI->r_CustomCharacterAssembly, headMesh, hairMesh, genderId);

	// (2) Overlay cosmetic rows from DB.
	sqlite3* db = Database::GetConnection();
	int applied = 0;
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT d.equipped_slot, i.item_id "
			"FROM ga_character_devices d "
			"JOIN ga_players_inventory i ON i.id = d.inventory_id "
			"WHERE d.character_id = ? AND d.item_profile_id = ? AND i.item_id > 0",
			-1, &stmt, nullptr);
		if (rc == SQLITE_OK) {
			sqlite3_bind_int64(stmt, 1, character_id);
			sqlite3_bind_int  (stmt, 2, item_profile_id);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				const int slot   = sqlite3_column_int(stmt, 0);
				const int itemId = sqlite3_column_int(stmt, 1);
				Logger::Log("spawn-asm",
					"LoadFromDB row: char=%lld itemProf=%d slot=%d itemId=%d -> ApplyOnly\n",
					(long long)character_id, item_profile_id, slot, itemId);
				if (ApplyOnly(Pawn, slot, itemId)) ++applied;
				SpawnAsmSnapshot("[post-ApplyOnly pawn]", Pawn,
					((ATgPawn_Character*)Pawn)->r_CustomCharacterAssembly);
			}
			sqlite3_finalize(stmt);
		} else {
			Logger::Log("cosmetic-equip", "LoadFromDB: prepare failed: %s\n",
				sqlite3_errmsg(db));
		}
	}

	// (3) Size the server collision cylinder from the resolved visual mesh.
	// Do not write r_nBodyMeshAsmId or call ApplyPawnSetup here: both paths
	// re-enter native pawn setup and crashed later loadout-profile switches.
	ApplyClassVisualFallback(Pawn, profileId);
	// 1225 is used ONLY as a collision-lookup key when no cosmetic suit is
	// equipped — it must NOT be written into SuitMeshId (the replicated,
	// rendered field), or "no suit" would render the Medic Acolyte mesh
	// instead of the bare class-default body.
	int collisionLookupMesh = charPawn->r_CustomCharacterAssembly.SuitMeshId;
	if (collisionLookupMesh <= 0) {
		collisionLookupMesh = 1225;
	}
	ApplyMeshCollisionData(Pawn, collisionLookupMesh);
	if (PRI) {
		// The pawn is authoritative; clear-only profile switches must not
		// leave PRI dye/trail fields from the previous profile.
		PRI->r_CustomCharacterAssembly = charPawn->r_CustomCharacterAssembly;
	}

	// (4) Force-include both pawn and PRI in the next net update so their
	// mirrored r_CustomCharacterAssembly fields land in the initial bunch
	// together. Without this the PRI's update is gated by the engine's normal
	// net-frequency timer, and UpdateCharacterAssetRefs on the client can fire
	// from stale defaults before the real cosmetic asset set is preloaded.
	Pawn->bNetDirty       = 1;
	Pawn->bForceNetUpdate = 1;
	if (PRI) {
		PRI->bNetDirty       = 1;
		PRI->bForceNetUpdate = 1;
	}

	Logger::Log("cosmetic-equip",
		"LoadFromDB: char=%lld itemProf=%d applied %d cosmetic(s); r_nBodyMeshAsmId=%d "
		"SuitMeshId=%d SuitFlairId=%d HelmetMeshId=%d HeadFlairId=%d JetpackTrailId=%d "
		"Dye=[%d,%d,%d,%d,%d]\n",
		(long long)character_id, item_profile_id, applied,
		Pawn->r_nBodyMeshAsmId,
		charPawn->r_CustomCharacterAssembly.SuitMeshId,
		charPawn->r_CustomCharacterAssembly.SuitFlairId,
		charPawn->r_CustomCharacterAssembly.HelmetMeshId,
		charPawn->r_CustomCharacterAssembly.HeadFlairId,
		charPawn->r_CustomCharacterAssembly.JetpackTrailId,
		charPawn->r_CustomCharacterAssembly.DyeList[0],
		charPawn->r_CustomCharacterAssembly.DyeList[1],
		charPawn->r_CustomCharacterAssembly.DyeList[2],
		charPawn->r_CustomCharacterAssembly.DyeList[3],
		charPawn->r_CustomCharacterAssembly.DyeList[4]);
}

void ClearUnsetSlots(ATgPawn* Pawn, const std::set<int>& equippedEngineSlots) {
	if (!Pawn) return;
	auto* charPawn = (ATgPawn_Character*)Pawn;
	auto* PRI      = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	auto has = [&](int slot) { return equippedEngineSlots.count(slot) > 0; };
	int cleared = 0;

	// Helmet (engine slot 12).
	if (!has(12)) {
		if (charPawn->r_CustomCharacterAssembly.HeadFlairId != -1 ||
		    charPawn->r_CustomCharacterAssembly.HelmetMeshId != -1) {
			charPawn->r_CustomCharacterAssembly.HeadFlairId = -1;
			charPawn->r_CustomCharacterAssembly.HelmetMeshId = -1;
			if (PRI) {
				PRI->r_CustomCharacterAssembly.HeadFlairId = -1;
				PRI->r_CustomCharacterAssembly.HelmetMeshId = -1;
			}
			++cleared;
		}
	}
	// Suit flair (engine slot 6).
	if (!has(6)) {
		if (charPawn->r_CustomCharacterAssembly.SuitFlairId != -1) {
			charPawn->r_CustomCharacterAssembly.SuitFlairId = -1;
			charPawn->r_CustomCharacterAssembly.SuitMeshId = -1;
			if (PRI) PRI->r_CustomCharacterAssembly.SuitFlairId = -1;
			++cleared;
		}
	}
	// Dyes (engine slots 16-20 → DyeList[0..4]).
	for (int slot = 16; slot <= 20; ++slot) {
		const int eSlot = slot - 16;
		if (!has(slot)) {
			if (charPawn->r_CustomCharacterAssembly.DyeList[eSlot] != kNoCosmeticItemId) {
				charPawn->r_CustomCharacterAssembly.DyeList[eSlot] = kNoCosmeticItemId;
				if (PRI) PRI->r_CustomCharacterAssembly.DyeList[eSlot] = kNoCosmeticItemId;
				++cleared;
			}
		}
	}
	// Jetpack trail (engine slot 21).
	if (!has(21)) {
		if (charPawn->r_CustomCharacterAssembly.JetpackTrailId != kNoCosmeticItemId) {
			charPawn->r_CustomCharacterAssembly.JetpackTrailId = kNoCosmeticItemId;
			if (PRI) PRI->r_CustomCharacterAssembly.JetpackTrailId = kNoCosmeticItemId;
			++cleared;
		}
	}

	if (cleared > 0) {
		ApplyClassVisualFallback(Pawn, charPawn->r_nProfileId);
		ApplyMeshCollisionData(Pawn, charPawn->r_CustomCharacterAssembly.SuitMeshId);
		if (PRI) PRI->r_CustomCharacterAssembly = charPawn->r_CustomCharacterAssembly;
		Pawn->bNetDirty       = 1;
		Pawn->bForceNetUpdate = 1;
		if (PRI) {
			PRI->bNetDirty       = 1;
			PRI->bForceNetUpdate = 1;
		}
		Logger::Log("loadout",
			"[ClearUnsetSlots] reset %d cosmetic field(s) to profile defaults (equipped slots=%d)\n",
			cleared, (int)equippedEngineSlots.size());
	}
}

}  // namespace CosmeticEquip
