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

