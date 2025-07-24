#pragma once
#include "framework.h"
#include "FortGameModeAthena.h"

namespace NetDriver {
	void (*TickFlushOG)(UNetDriver* This, float DeltaSeconds);
	void TickFlush(UNetDriver* This, float DeltaSeconds) {
		if (!This)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (This->ClientConnections.Num() > 0) {
			ServerReplicateActors(This->ReplicationDriver, DeltaSeconds);
		}

		if (GameState->WarmupCountdownEndTime - UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) <= 0 && GameState->GamePhase == EAthenaGamePhase::Warmup)
		{
			FortGameModeAthena::StartAircraftPhase(GameMode, 0);
		}

		return TickFlushOG(This, DeltaSeconds); //bro forgot to add return
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