#pragma once
#include "framework.h"

namespace NetDriver {
	void (*TickFlushOG)(UNetDriver* This, float DeltaSeconds);
	void TickFlush(UNetDriver* This, float DeltaSeconds) {
		if (This->ClientConnections.Num() > 0) {
			ServerReplicateActors(This, DeltaSeconds);
		}
	}

	float GetMaxTickRate(float DeltaTime, bool bAllowFrameRateSmoothing = true) {
		return 30.f;
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0xE4043C), TickFlush, (LPVOID*)&TickFlushOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x104914C), GetMaxTickRate, nullptr);

		Log("NetDriver Hooked!");
	}
}