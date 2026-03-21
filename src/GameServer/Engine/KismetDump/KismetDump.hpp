#pragma once
#include "src/pch.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Walk all USequenceOp instances in GObjObjects and log them to the "kismet" channel.
// Safe to call at any time; logs sub-level sequences if called after they've been streamed in.
inline void DumpKismetSequence() {
	return;
	TArray<UObject*>* objs = UObject::GObjObjects();
	if (!objs) return;

	UClass* seqOpClass     = USequenceOp::StaticClass();
	UClass* seqClass       = USequence::StaticClass();
	UClass* remEvtActClass = USeqAct_ActivateRemoteEvent::StaticClass();
	UClass* remEvtEvClass  = USeqEvent_RemoteEvent::StaticClass();
	if (!seqOpClass) return;

	Logger::Log("kismet", "=== KISMET SEQUENCE DUMP ===\n");

	for (int i = 0; i < objs->Count; i++) {
		UObject* obj = objs->Data[i];
		if (!obj || !obj->IsA(seqOpClass)) continue;
		if (obj->IsA(seqClass)) continue; // skip containers

		USequenceOp* op = (USequenceOp*)obj;
		const char* className = obj->Class ? obj->Class->GetFullName() : "?";
		const char* opName    = obj->GetFullName();

		// For remote-event ops, include the event name
		char extraInfo[128] = "";
		if (remEvtActClass && obj->IsA(remEvtActClass)) {
			FName& evName = ((USeqAct_ActivateRemoteEvent*)obj)->EventName;
			if (evName.Index > 0 && evName.Index < FName::Names()->Count) {
				snprintf(extraInfo, sizeof(extraInfo), " [RemoteEvent=%s]",
					FName::Names()->Data[evName.Index]->Name);
			}
		} else if (remEvtEvClass && obj->IsA(remEvtEvClass)) {
			FName& evName = ((USeqEvent_RemoteEvent*)obj)->EventName;
			if (evName.Index > 0 && evName.Index < FName::Names()->Count) {
				snprintf(extraInfo, sizeof(extraInfo), " [ListensFor=%s]",
					FName::Names()->Data[evName.Index]->Name);
			}
		}

		// Output connections
		char connections[1024] = "";
		int pos = 0;
		for (int ol = 0; ol < op->OutputLinks.Count && pos < (int)sizeof(connections) - 2; ol++) {
			FSeqOpOutputLink& outLink = op->OutputLinks.Data[ol];
			char descBuf[64] = "Out";
			if (outLink.LinkDesc.Data && outLink.LinkDesc.Count > 0) {
				wcstombs(descBuf, outLink.LinkDesc.Data, sizeof(descBuf) - 1);
				descBuf[sizeof(descBuf) - 1] = '\0';
			}
			for (int k = 0; k < outLink.Links.Count && pos < (int)sizeof(connections) - 2; k++) {
				USequenceOp* target = outLink.Links.Data[k].LinkedOp;
				if (!target) continue;
				int written = snprintf(connections + pos, sizeof(connections) - pos,
					" [%s]->%s", descBuf, target->GetFullName());
				if (written > 0) pos += written;
			}
		}

		Logger::Log("kismet", "  %s%s  |class| %s%s\n", opName, extraInfo, className, connections);
	}

	Logger::Log("kismet", "=== END KISMET DUMP ===\n");
}
