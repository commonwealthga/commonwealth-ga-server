#include "src/GameServer/Engine/KismetWebDump/KismetWebDump.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"
#include "lib/nlohmann/json.hpp"

#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using nlohmann::json;
using MapDumpWriters::FStringToUtf8;
using MapDumpWriters::FNameStr;
using MapDumpWriters::ExtractClassName;

namespace {

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

std::string FullNameOf(UObject* obj) {
	if (!obj) return std::string();
	const char* raw = obj->GetFullName();
	return std::string(raw ? raw : "<null>");
}

// Node identity for the JSON graph. GetFullName() on this build drops the
// instance-number suffix ("SeqVar_Object_57" prints as "SeqVar_Object"), so
// same-class siblings collide; qualify with the object pointer.
std::string SeqObjId(UObject* obj) {
	if (!obj) return std::string();
	char buf[16];
	snprintf(buf, sizeof(buf), "@%08x", (unsigned int)(uintptr_t)obj);
	return FullNameOf(obj) + buf;
}

// Short class name ("SeqAct_Delay") from the cached class full name.
std::string ShortClassOf(UObject* obj) {
	const std::string& full = ObjectClassCache::GetClassName(obj);
	return ExtractClassName(full.c_str());
}

std::string ColorHex(const FColor& c) {
	char buf[16];
	snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.R, c.G, c.B);
	return std::string(buf);
}

// Node taxonomy by class-name prefix (IsA is unreliable on this build).
enum class NodeKind { Event, Action, Condition, Variable, SubSequence, Frame, Other };

NodeKind ClassifyNode(const std::string& shortCls) {
	auto startsWith = [&](const char* p) {
		return shortCls.compare(0, strlen(p), p) == 0;
	};
	if (shortCls == "Sequence" || shortCls == "PrefabSequence") return NodeKind::SubSequence;
	if (startsWith("SequenceFrame")) return NodeKind::Frame;
	if (startsWith("SeqEvent_") || startsWith("TgSeqEvent_")
	 || startsWith("DEPRECATED_SeqEvent_")) return NodeKind::Event;
	if (startsWith("SeqCond_") || startsWith("TgSeqCond_")) return NodeKind::Condition;
	if (startsWith("SeqAct_") || startsWith("TgSeqAct_")) return NodeKind::Action;
	if (startsWith("SeqVar_") || startsWith("TgSeqVar_") || shortCls == "InterpData")
		return NodeKind::Variable;
	return NodeKind::Other;
}

const char* KindStr(NodeKind k) {
	switch (k) {
		case NodeKind::Event:       return "event";
		case NodeKind::Action:      return "action";
		case NodeKind::Condition:   return "condition";
		case NodeKind::Variable:    return "variable";
		case NodeKind::SubSequence: return "sequence";
		case NodeKind::Frame:       return "frame";
		default:                    return "other";
	}
}

// ---------------------------------------------------------------------------
// Generic property reflection (self-discovered offsets)
//
// The SDK headers leave UProperty/UStruct opaque, so the dump discovers the
// two slots it needs at runtime by probing properties whose Offset values we
// know from the generated headers. If discovery fails, the dump degrades to
// the structural fields only.
// ---------------------------------------------------------------------------

struct Reflection {
	bool ok        = false;
	int  offSlot   = -1;  // byte offset inside UProperty of the Offset field
	int  maskSlot  = -1;  // byte offset inside UProperty of UBoolProperty's BitMask
	int  childSlot = -1;  // byte offset inside UClass of the Children head pointer
};

int ReadSlot(void* obj, int slot) {
	return *(int*)((char*)obj + slot);
}

// Find the single slot in [lo, hi) where every (object, expectedValue) probe
// matches. Returns -1 unless exactly one slot satisfies all probes.
int DiscoverSlot(const std::vector<std::pair<void*, int>>& probes, int lo, int hi) {
	int found = -1;
	for (int slot = lo; slot < hi; slot += 4) {
		bool all = true;
		for (const auto& p : probes) {
			if (ReadSlot(p.first, slot) != p.second) { all = false; break; }
		}
		if (all) {
			if (found != -1) return -1;  // ambiguous
			found = slot;
		}
	}
	return found;
}

Reflection DiscoverReflection(const std::unordered_set<UObject*>& allObjs) {
	Reflection r;

	UObject* propIntValue = ObjectCache::Find("IntProperty Engine.SeqVar_Int.IntValue");
	UObject* propMaxTrig  = ObjectCache::Find("IntProperty Engine.SequenceEvent.MaxTriggerCount");
	UObject* propPosX     = ObjectCache::Find("IntProperty Engine.SequenceObject.ObjPosX");
	UObject* propPlayer   = ObjectCache::Find("BoolProperty Engine.SequenceEvent.bPlayerOnly");
	UObject* propClient   = ObjectCache::Find("BoolProperty Engine.SequenceEvent.bClientSideOnly");
	if (!propIntValue || !propMaxTrig || !propPosX || !propPlayer || !propClient) {
		Logger::Log("kismet", "[KismetWebDump] reflection probe properties not found, generic props disabled\n");
		return r;
	}

	// UProperty payload lives in [0x44, 0x84) on this build (UStructProperty
	// adds its own field at 0x84).
	r.offSlot = DiscoverSlot({
		{ propIntValue, 0x94 },   // USeqVar_Int::IntValue
		{ propMaxTrig,  0xEC },   // USequenceEvent::MaxTriggerCount
		{ propPosX,     0x44 },   // USequenceObject::ObjPosX
	}, 0x44, 0x84);
	if (r.offSlot == -1) {
		Logger::Log("kismet", "[KismetWebDump] UProperty::Offset slot not discovered, generic props disabled\n");
		return r;
	}
	// Both bool probes share Offset 0xF4 but differ in BitMask (0x2 vs 0x8) —
	// sanity-check Offset, then find the mask slot.
	if (ReadSlot(propPlayer, r.offSlot) != 0xF4 || ReadSlot(propClient, r.offSlot) != 0xF4) {
		Logger::Log("kismet", "[KismetWebDump] bool probe offset mismatch, generic props disabled\n");
		return r;
	}
	for (int slot = 0x44; slot < 0x84; slot += 4) {
		if (slot == r.offSlot) continue;
		if (ReadSlot(propPlayer, slot) == 0x2 && ReadSlot(propClient, slot) == 0x8) {
			if (r.maskSlot != -1) { r.maskSlot = -1; break; }  // ambiguous
			r.maskSlot = slot;
		}
	}

	// UClass::Children: a pointer to a UField whose Outer is the class itself.
	// PropertyLink heads satisfy the same test, so pick the candidate whose
	// Next-chain is longest (Children starts at the FIRST child and is a
	// superset of any link-chain suffix). Verified on two classes.
	UClass* clsSeqEvent = ClassPreloader::GetClass("Class Engine.SequenceEvent");
	UClass* clsSeqVarInt = ClassPreloader::GetClass("Class Engine.SeqVar_Int");
	if (!clsSeqEvent || !clsSeqVarInt) return r;

	auto chainLen = [&](UClass* cls, int slot) -> int {
		UObject* head = *(UObject**)((char*)cls + slot);
		if (!head || !allObjs.count(head)) return -1;
		int len = 0;
		UField* f = (UField*)head;
		while (f && len < 256) {
			if (!allObjs.count((UObject*)f)) return -1;
			if (f->Outer != (UObject*)cls) return -1;
			len++;
			f = f->Next;
		}
		return len;
	};

	int bestSlot = -1, bestTotal = -1;
	for (int slot = 0x44; slot < 0xA0; slot += 4) {
		int a = chainLen(clsSeqEvent, slot);
		int b = chainLen(clsSeqVarInt, slot);
		if (a <= 0 || b <= 0) continue;
		if (a + b > bestTotal) { bestTotal = a + b; bestSlot = slot; }
	}
	if (bestSlot == -1) {
		Logger::Log("kismet", "[KismetWebDump] UClass::Children slot not discovered, generic props disabled\n");
		return r;
	}
	r.childSlot = bestSlot;

	r.ok = true;
	Logger::Log("kismet",
		"[KismetWebDump] reflection discovered: UProperty::Offset @ +0x%x, BitMask @ +0x%x, UClass::Children @ +0x%x\n",
		r.offSlot, r.maskSlot, r.childSlot);
	return r;
}

// Map-object id via reflection: many Tg actor classes declare m_nMapObjectId
// at class-specific offsets (same field the map_* DB tables key on). Resolve
// the offset once per UClass; INT_MIN = class has no such field.
int MapObjectIdOf(UObject* obj, const Reflection& r,
                  const std::unordered_set<UObject*>& allObjs) {
	if (!obj || !r.ok) return INT_MIN;
	static std::unordered_map<UClass*, int> offsetCache;
	int off;
	auto it = offsetCache.find(obj->Class);
	if (it != offsetCache.end()) {
		off = it->second;
	} else {
		off = -1;
		for (UClass* cls = obj->Class; cls && off == -1;
		     cls = (UClass*)((UField*)cls)->SuperField) {
			if (!allObjs.count((UObject*)cls)) break;
			UField* f = *(UField**)((char*)cls + r.childSlot);
			int guard = 0;
			for (; f && guard < 512; f = f->Next, guard++) {
				if (!allObjs.count((UObject*)f)) break;
				const char* nm = FNameStr(((UObject*)f)->Name);
				if (nm && strcmp(nm, "m_nMapObjectId") == 0) {
					off = ReadSlot(f, r.offSlot);
					break;
				}
			}
		}
		offsetCache[obj->Class] = off;
	}
	if (off < 0 || off > 0x4000) return INT_MIN;
	return *(int*)((char*)obj + off);
}

// Classes whose fields are already dumped structurally — the generic walk
// stops when it reaches one of these.
bool IsStructuralBase(const std::string& shortCls) {
	return shortCls == "SequenceOp" || shortCls == "SequenceEvent"
	    || shortCls == "SequenceVariable" || shortCls == "SequenceObject"
	    || shortCls == "SequenceFrame" || shortCls == "Sequence"
	    || shortCls == "Object";
}

// Dump properties declared on obj's class hierarchy below the structural
// bases: leaf-most first. One json entry per property: {n, t, v}.
json DumpGenericProps(UObject* obj, const Reflection& r,
                      const std::unordered_set<UObject*>& allObjs) {
	json out = json::array();
	if (!r.ok) return out;

	for (UClass* cls = obj->Class; cls; cls = (UClass*)((UField*)cls)->SuperField) {
		std::string clsShort = ExtractClassName(ObjectClassCache::GetClassName(cls).c_str());
		if (IsStructuralBase(clsShort)) break;

		UField* f = *(UField**)((char*)cls + r.childSlot);
		int guard = 0;
		for (; f && guard < 256; f = f->Next, guard++) {
			if (!allObjs.count((UObject*)f)) break;
			std::string pcls = ShortClassOf((UObject*)f);
			if (pcls.size() < 9 || pcls.compare(pcls.size() - 8, 8, "Property") != 0)
				continue;  // UFunction / UConst / UEnum etc.

			// "IntProperty Engine.SeqVar_Int.IntValue" -> "IntValue"
			std::string pfull = FullNameOf((UObject*)f);
			size_t dot = pfull.rfind('.');
			std::string pname = (dot == std::string::npos) ? pfull : pfull.substr(dot + 1);

			int off = ReadSlot(f, r.offSlot);
			if (off < 0 || off > 0x4000) continue;  // sanity
			char* base = (char*)obj + off;

			json e;
			e["n"] = pname;
			if (pcls == "IntProperty") {
				e["t"] = "int";   e["v"] = *(int*)base;
			} else if (pcls == "FloatProperty") {
				e["t"] = "float"; e["v"] = *(float*)base;
			} else if (pcls == "BoolProperty") {
				if (r.maskSlot == -1) continue;
				int mask = ReadSlot(f, r.maskSlot);
				e["t"] = "bool";  e["v"] = (*(int*)base & mask) != 0;
			} else if (pcls == "ByteProperty") {
				e["t"] = "byte";  e["v"] = (int)*(unsigned char*)base;
			} else if (pcls == "NameProperty") {
				e["t"] = "name";  e["v"] = std::string(FNameStr(*(FName*)base));
			} else if (pcls == "StrProperty") {
				e["t"] = "str";   e["v"] = FStringToUtf8(*(FString*)base);
			} else if (pcls == "ObjectProperty" || pcls == "ClassProperty"
			        || pcls == "ComponentProperty") {
				UObject* val = *(UObject**)base;
				e["t"] = "object";
				e["v"] = (val && allObjs.count(val)) ? FullNameOf(val) : (val ? "<dangling>" : "None");
				if (val && allObjs.count(val)) {
					int mid = MapObjectIdOf(val, r, allObjs);
					if (mid != INT_MIN) e["mapId"] = mid;
				}
			} else if (pcls == "ArrayProperty") {
				int count = *(int*)(base + 4);  // TArray::Count
				if (count < 0 || count > 100000) count = -1;
				e["t"] = "array"; e["v"] = count;
			} else if (pcls == "StructProperty") {
				e["t"] = "struct"; e["v"] = "(struct)";
			} else {
				continue;  // delegates, interfaces, ...
			}
			out.push_back(e);
		}
	}
	return out;
}

// ---------------------------------------------------------------------------
// Structural dump
// ---------------------------------------------------------------------------

json DumpOpLinks(USequenceOp* op) {
	json j;

	json inputs = json::array();
	for (int i = 0; i < op->InputLinks.Count; i++) {
		FSeqOpInputLink& in = op->InputLinks.Data[i];
		json e;
		e["desc"]     = FStringToUtf8(in.LinkDesc);
		e["disabled"] = (bool)in.bDisabled;
		inputs.push_back(e);
	}
	j["inputs"] = inputs;

	json outputs = json::array();
	for (int i = 0; i < op->OutputLinks.Count; i++) {
		FSeqOpOutputLink& out = op->OutputLinks.Data[i];
		json e;
		e["desc"]     = FStringToUtf8(out.LinkDesc);
		e["disabled"] = (bool)out.bDisabled;
		e["impulse"]  = (bool)out.bHasImpulse;
		if (out.ActivateDelay != 0.0f) e["delay"] = out.ActivateDelay;
		json to = json::array();
		for (int k = 0; k < out.Links.Count; k++) {
			USequenceOp* target = out.Links.Data[k].LinkedOp;
			if (!target) continue;
			json t;
			t["node"]  = SeqObjId((UObject*)target);
			t["input"] = out.Links.Data[k].InputLinkIdx;
			to.push_back(t);
		}
		e["to"] = to;
		outputs.push_back(e);
	}
	j["outputs"] = outputs;

	json varLinks = json::array();
	for (int i = 0; i < op->VariableLinks.Count; i++) {
		FSeqVarLink& vl = op->VariableLinks.Data[i];
		json e;
		e["desc"]      = FStringToUtf8(vl.LinkDesc);
		e["expected"]  = vl.ExpectedType ? ExtractClassName(ObjectClassCache::GetClassName(vl.ExpectedType).c_str()) : "";
		e["writeable"] = (bool)vl.bWriteable;
		json vars = json::array();
		for (int k = 0; k < vl.LinkedVariables.Count; k++) {
			if (vl.LinkedVariables.Data[k])
				vars.push_back(SeqObjId((UObject*)vl.LinkedVariables.Data[k]));
		}
		e["vars"] = vars;
		varLinks.push_back(e);
	}
	j["varLinks"] = varLinks;

	json eventLinks = json::array();
	for (int i = 0; i < op->EventLinks.Count; i++) {
		FSeqEventLink& el = op->EventLinks.Data[i];
		json e;
		e["desc"] = FStringToUtf8(el.LinkDesc);
		json evs = json::array();
		for (int k = 0; k < el.LinkedEvents.Count; k++) {
			if (el.LinkedEvents.Data[k])
				evs.push_back(SeqObjId((UObject*)el.LinkedEvents.Data[k]));
		}
		e["events"] = evs;
		eventLinks.push_back(e);
	}
	j["eventLinks"] = eventLinks;

	j["active"]        = (bool)op->bActive;
	j["activateCount"] = op->ActivateCount;
	return j;
}

json DumpVarValue(USequenceVariable* var, const std::string& shortCls,
                  const Reflection& r, const std::unordered_set<UObject*>& allObjs) {
	json v;
	v["varName"] = std::string(FNameStr(var->VarName));
	if (shortCls == "SeqVar_Int") {
		v["type"]  = "int";
		v["value"] = ((USeqVar_Int*)var)->IntValue;
	} else if (shortCls == "SeqVar_Float" || shortCls == "SeqVar_RandomFloat") {
		v["type"]  = "float";
		v["value"] = ((USeqVar_Float*)var)->FloatValue;
	} else if (shortCls == "SeqVar_Bool") {
		v["type"]  = "bool";
		v["value"] = ((USeqVar_Bool*)var)->bValue != 0;
	} else if (shortCls == "SeqVar_Byte") {
		v["type"]  = "byte";
		v["value"] = (int)((USeqVar_Byte*)var)->ByteValue;
	} else if (shortCls == "SeqVar_String") {
		v["type"]  = "string";
		v["value"] = FStringToUtf8(((USeqVar_String*)var)->StrValue);
	} else if (shortCls == "SeqVar_Named") {
		USeqVar_Named* n = (USeqVar_Named*)var;
		v["type"]  = "named";
		v["value"] = std::string(FNameStr(n->FindVarName));
		if (n->ExpectedType)
			v["expected"] = ExtractClassName(ObjectClassCache::GetClassName(n->ExpectedType).c_str());
	} else if (shortCls == "SeqVar_Object" || ObjectClassCache::ClassNameContains(var, "SeqVar_Object")) {
		UObject* obj = ((USeqVar_Object*)var)->ObjValue;
		v["type"]  = "object";
		v["value"] = obj ? FullNameOf(obj) : "None";
		if (obj && allObjs.count(obj)) {
			int mid = MapObjectIdOf(obj, r, allObjs);
			if (mid != INT_MIN) v["mapObjectId"] = mid;
		}
	} else if (shortCls == "InterpData") {
		v["type"]  = "interpdata";
		v["value"] = ((UInterpData*)var)->InterpLength;
	} else {
		v["type"]  = "unknown";
		v["value"] = nullptr;
	}
	return v;
}

}  // namespace

// ---------------------------------------------------------------------------

namespace {
// Delayed-dump arm state. Single-threaded in-game; re-armed each BeginPlay.
bool  s_dumpArmed   = false;
float s_dumpElapsed = 0.0f;
// Seconds to wait after BeginPlay for async sublevel streaming to settle before
// dumping. Sound/terrain sublevels (_Sound/_TER) finish loading well within
// this; bump if a larger map streams slower.
constexpr float kDumpSettleSeconds = 20.0f;
}  // namespace

void KismetWebDump::ArmDelayedDump() {
	s_dumpArmed   = true;
	s_dumpElapsed = 0.0f;
	Logger::Log("kismet",
		"[KismetWebDump] armed — will dump in %.0fs (after sublevel streaming settles)\n",
		kDumpSettleSeconds);
}

void KismetWebDump::TickDelayedDump(float deltaSeconds) {
	if (!s_dumpArmed) return;
	s_dumpElapsed += deltaSeconds;
	if (s_dumpElapsed < kDumpSettleSeconds) return;
	s_dumpArmed = false;  // one-shot
	DumpAll();
}

void KismetWebDump::DumpAll() {
	TArray<UObject*>* objs = UObject::GObjObjects();
	if (!objs) {
		Logger::Log("kismet", "[KismetWebDump] GObjObjects() is null, aborting\n");
		return;
	}

	std::string mapName = Config::GetMapNameChar();
	Logger::Log("kismet", "[KismetWebDump] BEGIN map=%s\n", mapName.c_str());

	// Single sweep: pointer identity set (validates every cross-reference we
	// emit) + the sequence containers.
	std::unordered_set<UObject*> allObjs;
	std::vector<USequence*> sequences;
	allObjs.reserve(objs->Count);
	for (int i = 0; i < objs->Count; i++) {
		UObject* obj = objs->Data[i];
		if (!obj) continue;
		allObjs.insert(obj);
	}
	for (int i = 0; i < objs->Count; i++) {
		UObject* obj = objs->Data[i];
		if (!obj || !obj->Class) continue;
		const std::string& cls = ObjectClassCache::GetClassName(obj);
		if (cls != "Class Engine.Sequence" && cls != "Class Engine.PrefabSequence") continue;
		std::string full = FullNameOf(obj);
		if (full.find("Default__") != std::string::npos) continue;
		sequences.push_back((USequence*)obj);
	}

	Reflection reflect = DiscoverReflection(allObjs);

	json root;
	root["map"]     = mapName;
	root["format"]  = 1;
	root["reflect"] = reflect.ok;

	json jSequences = json::array();
	json jNodes     = json::array();
	std::unordered_set<UObject*> visited;
	int nodeCount = 0;

	for (USequence* seq : sequences) {
		json js;
		js["id"]      = SeqObjId((UObject*)seq);
		js["name"]    = FStringToUtf8(seq->ObjName);
		js["parent"]  = seq->ParentSequence ? SeqObjId((UObject*)seq->ParentSequence) : "";
		js["enabled"] = (bool)seq->bEnabled;
		js["viewX"]   = seq->DefaultViewX;
		js["viewY"]   = seq->DefaultViewY;
		js["viewZoom"] = seq->DefaultViewZoom;
		jSequences.push_back(js);

		for (int i = 0; i < seq->SequenceObjects.Count; i++) {
			USequenceObject* node = seq->SequenceObjects.Data[i];
			if (!node || visited.count((UObject*)node)) continue;
			visited.insert((UObject*)node);

			std::string shortCls = ShortClassOf((UObject*)node);
			NodeKind kind = ClassifyNode(shortCls);

			json jn;
			jn["id"]      = SeqObjId((UObject*)node);
			jn["seq"]     = SeqObjId((UObject*)seq);
			jn["type"]    = KindStr(kind);
			jn["cls"]     = shortCls;
			jn["name"]    = FStringToUtf8(node->ObjName);
			jn["comment"] = FStringToUtf8(node->ObjComment);
			jn["x"]       = node->ObjPosX;
			jn["y"]       = node->ObjPosY;
			jn["color"]   = ColorHex(node->ObjColor);

			switch (kind) {
				case NodeKind::Event: {
					USequenceEvent* ev = (USequenceEvent*)node;
					jn["links"] = DumpOpLinks((USequenceOp*)node);
					json je;
					je["originator"]   = ev->Originator ? FullNameOf((UObject*)ev->Originator) : "None";
					if (ev->Originator && allObjs.count((UObject*)ev->Originator)) {
						int mid = MapObjectIdOf((UObject*)ev->Originator, reflect, allObjs);
						if (mid != INT_MIN) je["originatorMapId"] = mid;
					}
					je["maxTrig"]      = ev->MaxTriggerCount;
					je["trigCount"]    = ev->TriggerCount;
					je["reTrigDelay"]  = ev->ReTriggerDelay;
					je["enabled"]      = (bool)ev->bEnabled;
					je["playerOnly"]   = (bool)ev->bPlayerOnly;
					je["clientOnly"]   = (bool)ev->bClientSideOnly;
					je["priority"]     = (int)ev->Priority;
					jn["event"] = je;
					jn["props"] = DumpGenericProps((UObject*)node, reflect, allObjs);
					break;
				}
				case NodeKind::Action:
				case NodeKind::Condition:
				case NodeKind::SubSequence:
					jn["links"] = DumpOpLinks((USequenceOp*)node);
					if (kind != NodeKind::SubSequence)
						jn["props"] = DumpGenericProps((UObject*)node, reflect, allObjs);
					break;
				case NodeKind::Variable:
					jn["var"]   = DumpVarValue((USequenceVariable*)node, shortCls, reflect, allObjs);
					jn["props"] = DumpGenericProps((UObject*)node, reflect, allObjs);
					break;
				case NodeKind::Frame: {
					USequenceFrame* fr = (USequenceFrame*)node;
					json jf;
					jf["w"]           = fr->SizeX;
					jf["h"]           = fr->SizeY;
					jf["drawBox"]     = (bool)fr->bDrawBox;
					jf["filled"]      = (bool)fr->bFilled;
					jf["borderWidth"] = fr->BorderWidth;
					jf["borderColor"] = ColorHex(fr->BorderColor);
					jf["fillColor"]   = ColorHex(fr->FillColor);
					jn["frame"] = jf;
					break;
				}
				default:
					break;
			}

			jNodes.push_back(jn);
			nodeCount++;
		}
	}

	root["sequences"] = jSequences;
	root["nodes"]     = jNodes;

	std::string path = Config::GetLogDir() + "\\kismet_" + mapName + ".json";
	FILE* f = fopen(path.c_str(), "wb");
	if (!f) {
		Logger::Log("kismet", "[KismetWebDump] cannot open %s for writing\n", path.c_str());
		return;
	}
	std::string out = root.dump();
	fwrite(out.data(), 1, out.size(), f);
	fclose(f);

	Logger::Log("kismet", "[KismetWebDump] END map=%s sequences=%d nodes=%d reflect=%d -> %s\n",
		mapName.c_str(), (int)sequences.size(), nodeCount, (int)reflect.ok, path.c_str());
}
