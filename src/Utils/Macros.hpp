#pragma once

// (Former TARRAY_INIT / TARRAY_ADD macros removed — they libc-malloc'd into
//  UProperty TArray fields and survived only because their host UObjects were
//  long-lived. Now that `src/SDK/SdkHeaders.h::TArray::Add` routes through
//  GAllocator::Realloc, just use the SDK field directly:
//      Game->s_AttackerReviveList.Add(Controller);
//      BotFactory->m_SpawnQueue.Add(entry);
//      NetDriver->ClientConnections.Add(Connection);
//  Fall back to a raw `(TArray<T>*)((char*)base + offset)` cast only when the
//  SDK header doesn't expose the field.)

// DO_REP / DO_REP_ARRAY mirror UE3's DOREP / DOREPARRAY in UnNet.h:173-202.
//
// The initial-send gate is `Channel->OpenPacketId == INDEX_NONE && (sp->PropertyFlags & CPF_Config)`
// — only CPF_Config-marked properties are force-sent on a freshly opened channel
// (their CDO default may differ between client and server, invalidating the
// Recent==CDO comparison the rest of the rep system depends on). Every other
// property relies on `NEQ(current, Recent)` alone — and on initial channel-open
// `Recent` IS the archetype/CDO, so any property whose live value differs from
// the CDO default gets sent naturally.
//
// Wire layout: param_5 = UActorChannel*, +0x4c = OpenPacketId (compared to -1 = INDEX_NONE).
// UProperty layout: propertyfullname + 0x4c = PropertyFlags (CPF_Config = 0x4000),
//                   propertyfullname + 0x5e = RepIndex (uint16_t).
//
// NEQ arg order mirrors UE3 `DOREP(c,v)` — first arg is the CURRENT actor value
// (the candidate we want to send), second arg is recent/last-replicated. The
// `NEQ(UObject*, ...)` overload calls CanSerializeObject(A) to mark the channel
// "stay dirty" when the current value's NetGUID hasn't yet resolved on the
// receiver; argument order matters for that side effect.
#define DO_REP(actortype, propertyname, propertyfullname) \
	{ \
		if (propertyfullname != nullptr && \
		    ((*(int *)(param_5 + 0x4c) == -1 && (*(unsigned int *)((int)propertyfullname + 0x4c) & 0x4000) != 0) \
		     || NEQ(((actortype*)actor)->propertyname, ((actortype*)recent)->propertyname, (void*)param_4, (void*)param_5))) { \
			repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
			*param_3++ = repindex; \
		} \
	}

// Force-replicate every tick the value is non-zero, bypassing NEQ change-
// detection. For client-folded accumulator fields (e.g. r_fMakeVisibleIncreased)
// where change-detection would drop the stream.
#define DO_REP_FORCED(actortype, propertyname, propertyfullname) \
	{ \
		if (propertyfullname != nullptr && ((actortype*)actor)->propertyname != 0) { \
			repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
			*param_3++ = repindex; \
		} \
	}

// UE3 DOREPARRAY semantics: per-element NEQ, only emit changed indices. The
// CPF_Config + fresh-channel case force-emits every index.
#define DO_REP_ARRAY(count, actortype, propertyname, propertyfullname) \
	{ \
		if (propertyfullname != nullptr) { \
			const bool forceAllElements = \
				(*(int *)(param_5 + 0x4c) == -1) && \
				((*(unsigned int *)((int)propertyfullname + 0x4c) & 0x4000) != 0); \
			if (forceAllElements) { \
				for (int i = 0; i < count; i++) { \
					repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
					*param_3++ = repindex+i; \
				} \
			} else { \
				for (int i = 0; i < count; i++) { \
					if (NEQ(((actortype*)actor)->propertyname[i], ((actortype*)recent)->propertyname[i], (void*)param_4, (void*)param_5)) { \
						repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
						*param_3++ = repindex+i; \
					} \
				} \
			} \
		} \
	}

