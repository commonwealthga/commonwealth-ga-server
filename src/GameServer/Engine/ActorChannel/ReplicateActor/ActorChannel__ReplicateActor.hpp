#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UActorChannel::ReplicateActor — verified at 0x10d8c070 (Ghidra decompile).
// void __thiscall, no args (this = UActorChannel* in ECX). This is the
// per-connection, per-actor replication entry point: the engine calls it once
// for each (connection, relevant actor) pair every replication pass, then
// serializes the actor's changed properties to THAT connection. That makes it
// the one clean seam for per-observer property divergence.
class ActorChannel__ReplicateActor : public HookBase<
	void(__fastcall*)(void*, void*),
	0x10D8C070,
	ActorChannel__ReplicateActor> {
public:
	static void __fastcall Call(void* Channel, void* edx);
	static inline void __fastcall CallOriginal(void* Channel, void* edx) {
		m_original(Channel, edx);
	}
};
