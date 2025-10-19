#pragma once

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

#define DO_REP(actortype, propertyname, propertyfullname) \
	{ \
		bool &initialFlag = propertyfullname##_initial[(int)actor]; \
		if (propertyfullname != nullptr && ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((actortype*)recent)->propertyname, ((actortype*)actor)->propertyname, (void*)param_4, (void*)param_5))) { \
			initialFlag = true; \
			repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
			*param_3++ = repindex; \
		} \
	}

#define DO_REP_ARRAY(count, actortype, propertyname, propertyfullname) \
	{ \
		bool &initialFlag = propertyfullname##_initial[(int)actor]; \
		if (propertyfullname != nullptr) { \
			for (int i = 0; i < count; i++) { \
				if ((*(int *)(param_5 + 0x4c) == -1 && !initialFlag) || NEQ(((actortype*)recent)->propertyname[i], ((actortype*)actor)->propertyname[i], (void*)param_4, (void*)param_5)) { \
						repindex = (int)*(short *)((int)propertyfullname + 0x5e); \
						*param_3++ = repindex+i; \
				} \
			} \
			initialFlag = true; \
		} \
	}

