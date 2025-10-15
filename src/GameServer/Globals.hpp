#pragma once

struct Globals {
	// sdk stuff
	void* GObjects = (void*)0x13465a54;
	void* GNames = (void*)0x13454180;

	// global objects
	void* GWorld = nullptr;
	void* GWorldInfo = nullptr;
	void* GAssemblyDatManager = (void*)0x1199f868;
	void* GTransientPackage = (void*)0x134659bc;
	void** ClientConnectionClass = (void**)0x119a1ae0;

	// vtables
	void** PackageMapVtable = (void**)0x11769a48;

	// global variables
	int* GUseSeekFreeLoading = (int*)0x13433908;
	int* GUseSeekFreePackageMap= (int*)0x134338E0;
	int* GIsClient = (int*)0x1342B844;
	int* GIsServer = (int*)0x1342B848;
	int* GIsEditor = (int*)0x1342B830;
	int* GIsUCC = (int*)0x1342B838;
	int* GUseNewOptimizedServerTick = (int*)0x119863b8;
	int* UObjectGObjFirstGCIndex = (int*)0x13465A10;

	static Globals& Get() {
		static Globals instance;
		return instance;
	}
};

