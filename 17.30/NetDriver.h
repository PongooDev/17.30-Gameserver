#pragma once
#include "framework.h"
#include "FortGameModeAthena.h"

namespace NetDriver {
	EAthenaGamePhaseStep GetCurrentGamePhaseStep(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState) {
		float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

		if (GameState->GamePhase == EAthenaGamePhase::Setup) {
			return EAthenaGamePhaseStep::Setup;
		}
		else if (GameState->GamePhase == EAthenaGamePhase::Warmup) {
			if (GameState->WarmupCountdownEndTime > CurrentTime + 10.f) {
				return EAthenaGamePhaseStep::Warmup;
			}
			else {
				return EAthenaGamePhaseStep::GetReady;
			}
		}
		else if (GameState->GamePhase == EAthenaGamePhase::Aircraft) {
			if (GameState->GamePhaseStep > EAthenaGamePhaseStep::BusLocked) {
				// We handle this in OnAircraftEnteredDropZone
				return GameState->GamePhaseStep;
			}
			else {
				return EAthenaGamePhaseStep::BusLocked;
			}
		}
		else if (GameState->GamePhase == EAthenaGamePhase::SafeZones) {
			if (!GameState->SafeZoneIndicator) {
				return EAthenaGamePhaseStep::StormForming;
			}
			else if (GameState->SafeZoneIndicator->bPaused) {
				return EAthenaGamePhaseStep::StormHolding;
			}
			else {
				return EAthenaGamePhaseStep::StormShrinking;
			}
		}
		else if (GameState->GamePhase == EAthenaGamePhase::EndGame) {
			return EAthenaGamePhaseStep::EndGame;
		}
		else if (GameState->GamePhase == EAthenaGamePhase::Count) {
			return EAthenaGamePhaseStep::Count;
		}
		else {
			return EAthenaGamePhaseStep::EAthenaGamePhaseStep_MAX;
		}
	}

	void UpdateBotBlackboard(AFortAthenaAIBotController* bot, AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState) {
		if (!bot)
			return;

		AFortPlayerStateAthena* botPS = (AFortPlayerStateAthena*)bot->PlayerState;
		if (botPS) {
			if (GameState->GamePhaseStep > EAthenaGamePhaseStep::GetReady) {
				bot->Blackboard->SetValueAsBool(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_IsInBus"), botPS->bInAircraft);
			}
		}

		bot->Blackboard->SetValueAsEnum(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_GamePhaseStep"), (int)GameState->GamePhaseStep);
	}

	void (*TickFlushOG)(UNetDriver* This, float DeltaSeconds);
	void TickFlush(UNetDriver* This, float DeltaSeconds) {
		if (!This)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (GameMode && GameState) {
			EAthenaGamePhaseStep CurrentGamePhaseStep = GetCurrentGamePhaseStep(GameMode, GameState);
			GameState->GamePhaseStep = CurrentGamePhaseStep;
			if (Globals::bBotsEnabled) {
				for (AFortAthenaAIBotController* bot : GameMode->AliveBots) {
					UpdateBotBlackboard(bot, GameMode, GameState);
				}
			}
		}

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