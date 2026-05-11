#pragma once

#include <unordered_set>

class AActor;

// One global record of "actors that have been seeded with their initial replication
// pass". Replaces the 511 per-property std::map<int,bool> ##_initial maps that used
// to live in the GetOptimizedRepListV2 file. Populated at the end of
// Actor__GetOptimizedRepList::Call (when the captured isInitialRep was true);
// queried at the top of every call to derive isInitialRep. DO_REP / DO_REP_ARRAY
// below read that local boolean instead of doing per-property red-black-tree
// lookups.
extern std::unordered_set<AActor*> g_RepListInitialDoneActors;

#define TARRAY_INIT(basename, arrayname, datatype, offset, max) \
    datatype** arrayname##DataPtr = (datatype**)((char*)basename + offset); \
    int* arrayname##CountPtr = (int*)((char*)basename + offset + 0x4); \
    int* arrayname##MaxPtr   = (int*)((char*)basename + offset + 0x8); \
    \
    if (*arrayname##DataPtr == nullptr) \
    { \
        *arrayname##DataPtr = (datatype*)malloc(sizeof(datatype) * max); \
        memset(*arrayname##DataPtr, 0, sizeof(datatype) * max); \
        *arrayname##CountPtr = 0; \
        *arrayname##MaxPtr = max; \
    }

#define TARRAY_ADD(arrayname, varname) \
    { \
        int Count = *arrayname##CountPtr; \
        (*arrayname##DataPtr)[Count] = varname; \
        *arrayname##CountPtr = Count + 1; \
    }

// DO_REP / DO_REP_ARRAY consume a local `bool isInitialRep` declared at the top of
// the dispatch function — true the first time this actor is seen by the rep list,
// false otherwise. Replaces the per-property per-actor std::map<int,bool> lookup
// the older implementation did.
#define DO_REP(actortype, propertyname, propertyfullname) \
	{ \
		if (propertyfullname != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && isInitialRep) || NEQ(((actortype*)recent)->propertyname, ((actortype*)actor)->propertyname, (void*)param_4, (void*)param_5))) { \
			repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
			*param_3++ = repindex; \
		} \
	}

#define DO_REP_ARRAY(count, actortype, propertyname, propertyfullname) \
	{ \
		if (propertyfullname != nullptr) { \
			if (isInitialRep) { \
				for (int i = 0; i < count; i++) { \
					repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
					*param_3++ = repindex+i; \
				} \
			} else { \
				bool bDoRep = false; \
				if (*(int *)(param_5 + 0x4c) == -1) { \
					bDoRep = true; \
				} else { \
					for (int i = 0; i < count; i++) { \
						if (NEQ(((actortype*)recent)->propertyname[i], ((actortype*)actor)->propertyname[i], (void*)param_4, (void*)param_5)) { \
							bDoRep = true; \
							break; \
						} \
					} \
				} \
				if (bDoRep) { \
					for (int i = 0; i < count; i++) { \
						repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
						*param_3++ = repindex+i; \
					} \
				} \
			} \
		} \
	}

