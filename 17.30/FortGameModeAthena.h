#pragma once
#include "framework.h"
#include "FortInventory.h"
#include "AbilitySystemComponent.h"

#include "PlayerBots.h"

namespace FortGameModeAthena {
	bool ReadyToStartMatch(AFortGameModeAthena* GameMode) {
		static bool bSetupPlaylist = false;
		static bool bInitialized = false;
		static bool bListening = false;

		if (!GameMode) {
			return false;
		}

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)GameMode->GameState;
		if (!GameState) {
			return false;
		}

		float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
		float WarmupTime = 60.f;

		if (!bSetupPlaylist) {
			bSetupPlaylist = true;
			UFortPlaylistAthena* Playlist = nullptr;
			if (Globals::bEventEnabled) {
				Playlist = StaticLoadObject<UFortPlaylistAthena>("/BuffetPlaylist/Playlist/Playlist_Buffet.Playlist_Buffet");
			}
			else {
				Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
			}
			if (!Playlist) {
				Log("Playlist Not Found!");
				return false;
			}

			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GameState->CurrentPlaylistInfo.MarkArrayDirty();
			GameState->OnRep_CurrentPlaylistInfo();

			GameMode->CurrentPlaylistName = Playlist->PlaylistName;
			GameState->CurrentPlaylistId = Playlist->PlaylistId;
			GameState->OnRep_CurrentPlaylistId();

			GameState->WarmupCountdownEndTime = CurrentTime + WarmupTime;
			GameMode->WarmupCountdownDuration = WarmupTime;
			GameState->WarmupCountdownStartTime = CurrentTime;
			GameMode->WarmupEarlyCountdownDuration = WarmupTime;

			GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;
			GameMode->GameSession->MaxSpectators = 0;
			GameMode->GameSession->MaxPartySize = Playlist->MaxSquadSize;
			GameMode->GameSession->MaxSplitscreensPerConnection = 2;
			GameMode->GameSession->bRequiresPushToTalk = false;
			GameMode->GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(FString(L"GameSession"));

			Globals::MaxPlayersPerTeam = Playlist->MaxSquadSize;
			Globals::NextTeamIndex = Playlist->DefaultFirstTeam;

			Log("Setup Playlist: " + Playlist->GetName());
		}

		if (!GameState->MapInfo) {
			return false;
		}

		if (!bInitialized) {
			bInitialized = true;

			GameState->OnRep_CurrentPlaylistId();
			GameState->OnRep_CurrentPlaylistInfo();

			GameMode->bAllowSpectateAfterDeath = true;
			GameMode->MinRespawnDelay = 5.0f;
			GameMode->WarmupRequiredPlayerCount = 1;

			GameState->DefaultParachuteDeployTraceForGroundDistance = 10000;

			if (auto BotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode))
			{
				GameMode->AISettings = GameState->CurrentPlaylistInfo.BasePlaylist->AISettings;
				GameMode->AISettings->AIServices[1] = UAthenaAIServicePlayerBots::StaticClass();

				GameMode->ServerBotManager = BotManager;
				BotManager->CachedGameState = GameState;
				BotManager->CachedGameMode = GameMode;

				*(bool*)(__int64(GameMode->ServerBotManager) + 0x458) = true;
				GameMode->ServerBotManager->CachedAIPopulationTracker = ((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AIPopulationTracker;

				if (!GameMode->SpawningPolicyManager)
				{
					GameMode->SpawningPolicyManager = SpawnActor<AFortAthenaSpawningPolicyManager>({}, {});
				}
				GameMode->SpawningPolicyManager->GameModeAthena = GameMode;
				GameMode->SpawningPolicyManager->GameStateAthena = GameState;

				if (GameMode->ServerBotManager->CachedBotMutator) {
					BotMutator = GameMode->ServerBotManager->CachedBotMutator;
				}

				if (!BotMutator)
				{
					BotMutator = (AFortAthenaMutator_Bots*)GameMode->GetMutatorByClass(GameMode, AFortAthenaMutator_Bots::StaticClass());
				}

				if (!BotMutator)
				{
					BotMutator = (AFortAthenaMutator_Bots*)GameMode->GetMutatorByClass(GameMode->GameState, AFortAthenaMutator_Bots::StaticClass());
				}

				if (!BotMutator) {
					BotMutator = SpawnActor<AFortAthenaMutator_Bots>({});
				}

				BotManager->CachedBotMutator = BotMutator;
				BotMutator->CachedGameMode = GameMode;
				BotMutator->CachedGameState = GameState;

				GameMode->AIDirector = SpawnActor<AAthenaAIDirector>({});
				if (GameMode->AIDirector) {
					//Log("AIDirector!");
					GameMode->AIDirector->Activate();
				}
				else {
					Log("No AIDirector!");
				}

				if (!GameMode->AIGoalManager)
				{
					GameMode->AIGoalManager = SpawnActor<AFortAIGoalManager>({});
				}

				UAISystem::GetDefaultObj()->AILoggingVerbose();

				//this is better (we wont have stuff that goes null anymore + dev stuff check)
				for (size_t i = 0; i < UObject::GObjects->Num(); i++)
				{
					UObject* Obj = UObject::GObjects->GetByIndex(i);
					if (Obj && Obj->IsA(UAthenaCharacterItemDefinition::StaticClass()))
					{
						std::string SkinsData = ((UAthenaCharacterItemDefinition*)Obj)->Name.ToString();

						if (SkinsData.contains("Athena_Commando") || SkinsData.contains("CID_Character") || !SkinsData.contains("CID_NPC") || !SkinsData.contains("CID_VIP") || !SkinsData.contains("CID_TBD"))
						{
							Characters.push_back((UAthenaCharacterItemDefinition*)Obj);
						}
					}
					if (Obj && Obj->IsA(UAthenaBackpackItemDefinition::StaticClass())) // pretty sure this doesnt have dev stuff
					{
						Backpacks.push_back((UAthenaBackpackItemDefinition*)Obj);
					}
					if (Obj && Obj->IsA(UAthenaPickaxeItemDefinition::StaticClass())) // pretty sure this doesnt have dev stuff
					{
						Pickaxes.push_back((UAthenaPickaxeItemDefinition*)Obj);
					}
					if (Obj && Obj->IsA(UAthenaDanceItemDefinition::StaticClass()))
					{
						std::string EmoteData = ((UAthenaDanceItemDefinition*)Obj)->Name.ToString();

						if (EmoteData.contains("EID") || !EmoteData.contains("Sync") || !EmoteData.contains("Owned"))
						{
							Dances.push_back((UAthenaDanceItemDefinition*)Obj);
						}

					}
					if (Obj && Obj->IsA(UAthenaGliderItemDefinition::StaticClass()))
					{
						Gliders.push_back((UAthenaGliderItemDefinition*)Obj);
					}
				}

				Log("Initialised Bots!");
			}
			else
			{
				Log("BotManager is nullptr!");
			}

			for (auto& Level : GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels)
			{
				bool Success = false;
				ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString());
				FAdditionalLevelStreamed level{};
				level.bIsServerOnly = false;
				level.LevelName = Level.ObjectID.AssetPathName;
				if (Success) GameState->AdditionalPlaylistLevelsStreamed.Add(level);
			}
			for (auto& Level : GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly)
			{
				bool Success = false;
				ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString());
				FAdditionalLevelStreamed level{};
				level.bIsServerOnly = true;
				level.LevelName = Level.ObjectID.AssetPathName;
				if (Success) GameState->AdditionalPlaylistLevelsStreamed.Add(level);
			}
			GameState->OnRep_AdditionalPlaylistLevelsStreamed();
			GameState->OnFinishedStreamingAdditionalPlaylistLevel();

			Log("Initialized!");
		}

		if (!bListening) {
			bListening = true;

			auto Beacon = SpawnActor<AFortOnlineBeaconHost>({});
			Beacon->ListenPort = 7777;
			InitHost(Beacon);
			PauseBeaconRequests(Beacon, false);

			UWorld::GetWorld()->NetDriver = Beacon->NetDriver;
			UWorld::GetWorld()->NetDriver->World = UWorld::GetWorld();
			UWorld::GetWorld()->NetDriver->NetDriverName = UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"GameNetDriver");

			FString Error = FString();
			FURL URL{};
			URL.Port = 7777;

			if (InitListen(UWorld::GetWorld()->NetDriver, UWorld::GetWorld(), URL, true, Error)) {
				Log("InitListen Successful!");
			}
			else {
				Log("InitListen Unsuccessful: " + Error.ToString());
			}

			SetWorld(UWorld::GetWorld()->NetDriver, UWorld::GetWorld());

			for (int i = 0; i < UWorld::GetWorld()->LevelCollections.Num(); i++) {
				UWorld::GetWorld()->LevelCollections[i].NetDriver = UWorld::GetWorld()->NetDriver;
			}

			GameMode->bWorldIsReady = true;

			Log("Listening!");
			SetConsoleTitleA("17.30 Gameserver | Listening");
		}

		if (GameState->PlayersLeft > 0) {
			UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingFoundation::StaticClass(), &BuildingFoundations);
			UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);

			return true;
		}
		else {
			GameState->WarmupCountdownEndTime = CurrentTime + WarmupTime;
			GameMode->WarmupCountdownDuration = WarmupTime;
			GameState->WarmupCountdownStartTime = CurrentTime;
			GameMode->WarmupEarlyCountdownDuration = WarmupTime;
		}

		return false;
	}

	APawn* (*SpawnDefaultPawnForOG)(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer, AActor* StartSpot);
	APawn* SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer, AActor* StartSpot) {
		Log("SpawnDefaultPawnFor Called!");
		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(NewPlayer, StartSpot->GetTransform());
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;

		NewPlayer->XPComponent->bRegisteredWithQuestManager = true;
		NewPlayer->XPComponent->OnRep_bRegisteredWithQuestManager();

		PlayerState->SeasonLevelUIDisplay = NewPlayer->XPComponent->CurrentLevel;
		PlayerState->OnRep_SeasonLevelUIDisplay();

		NewPlayer->GetQuestManager(ESubGame::Athena)->InitializeQuestAbilities(NewPlayer->Pawn);

		UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
		PlayerState->OnRep_CharacterData();

		UAthenaPickaxeItemDefinition* PickDef;
		FFortAthenaLoadout& CosmecticLoadoutPC = NewPlayer->CosmeticLoadoutPC;
		PickDef = CosmecticLoadoutPC.Pickaxe != nullptr ? CosmecticLoadoutPC.Pickaxe : StaticLoadObject<UAthenaPickaxeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
		if (PickDef) {
			FortInventory::GiveItem(NewPlayer, PickDef->WeaponDefinition, 1, 0);
		}
		else {
			Log("Pick Doesent Exist!");
		}

		for (size_t i = 0; i < GameMode->StartingItems.Num(); i++)
		{
			if (GameMode->StartingItems[i].Count > 0)
			{
				FortInventory::GiveItem(NewPlayer, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count, 0);
			}
		}

		AbilitySystemComponent::GiveAbilitySet(StaticLoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer"), PlayerState);
		//AbilitySystemComponent::GiveAbilitySet(StaticLoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_DefaultPlayer.GAS_DefaultPlayer"), PlayerState);

		ApplyCharacterCustomization(PlayerState, Pawn);

		return Pawn;
	}

	__int64 (*StartAircraftPhaseOG)(AFortGameModeAthena* GameMode, bool bUseAircraftCountdown);
	__int64 StartAircraftPhase(AFortGameModeAthena* GameMode, bool bUseAircraftCountdown) {
		Log("StartAircraftPhase Called!");
		for (PlayerBots::PhoebeBot* bot : PlayerBots::PhoebeBots) {
			AFortPlayerStateAthena* botPS = bot->PlayerState;
			botPS->bInAircraft = true;
			bot->PC->Blackboard->SetValueAsBool(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_IsInBus"), true);
		}

		return StartAircraftPhaseOG(GameMode, bUseAircraftCountdown);
	}

	static inline void (*OriginalOnAircraftExitedDropZone)(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft);
	void OnAircraftExitedDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft)
	{
		Log("OnAircraftExitedDropZone!");

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		for (PlayerBots::PhoebeBot* bot : PlayerBots::PhoebeBots) {
			if (!bot->PlayerState->bInAircraft) continue;

			FVector Destination = bot->PC->Blackboard->GetValueAsVector(ConvFName(L"AIEvaluator_JumpOffBus_Destination"));

			bot->PC->Blackboard->SetValueAsVector(ConvFName(L"AIEvaluator_Dive_Destination"), Destination);
			bot->PC->Blackboard->SetValueAsVector(ConvFName(L"AIEvaluator_Glide_Destination"), Destination);

			bot->Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
			bot->Pawn->BeginSkydiving(true);
			bot->Pawn->SetHealth(100);
			bot->Pawn->SetShield(0);

			bot->PlayerState->bInAircraft = false;

			bot->PC->Blackboard->SetValueAsBool(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_HasEverJumpedFromBusKey"), true);
			bot->PC->Blackboard->SetValueAsBool(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_HasEverJumpedFromBusAndLandedKey"), false);
			bot->PC->Blackboard->SetValueAsBool(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_IsInBus"), false);

			bot->PC->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Dive_ExecutionStatus"), (int)EExecutionStatus::ExecutionAllowed);
			bot->PC->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Glide_ExecutionStatus"), (int)EExecutionStatus::ExecutionDenied);
		}

		return OriginalOnAircraftExitedDropZone(GameMode, FortAthenaAircraft);
	}

	void (*OnAircraftEnteredDropZoneOG)(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft);
	void OnAircraftEnteredDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft)
	{
		Log("OnAircraftEnteredDropZone Called!");

		return OnAircraftEnteredDropZoneOG(GameMode, FortAthenaAircraft);
	}

	// We can make ts proper later yuh
	static inline void (*StartNewSafeZonePhaseOG)(AFortGameModeAthena* GameMode, int32 ZoneIndex);
	static void StartNewSafeZonePhase(AFortGameModeAthena* GameMode, int32 ZoneIndex) {
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)GameMode->GameState;

		FFortSafeZoneDefinition* SafeZoneDefinition = &GameState->MapInfo->SafeZoneDefinition;
		Log("SafeZonePhase: " + std::to_string(GameMode->SafeZonePhase));

		static UCurveTable* AthenaGameData = StaticLoadObject<UCurveTable>(UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->GameData.ObjectID.AssetPathName).ToString());
		if (!AthenaGameData) {
			AthenaGameData = StaticLoadObject<UCurveTable>("/Game/Athena/Balance/DataTables/AthenaGameData.AthenaGameData");
		}
		float CurrentWaitTime = 30.f;
		EEvaluateCurveTableResult WaitTimeResult;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(AthenaGameData, UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.WaitTime"), (GameMode->SafeZonePhase + 1), &WaitTimeResult, &CurrentWaitTime, FString());
		if (WaitTimeResult == EEvaluateCurveTableResult::RowNotFound) {
			Log("Not Found WaitTime Row!");
		}
		else {
			GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + CurrentWaitTime;
		}

		float CurrentShrinkTime = 30.f;
		EEvaluateCurveTableResult ShrinkTimeResult;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(AthenaGameData, UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.ShrinkTime"), (GameMode->SafeZonePhase + 1), &ShrinkTimeResult, &CurrentShrinkTime, FString());
		if (ShrinkTimeResult == EEvaluateCurveTableResult::RowNotFound) {
			Log("Not Found ShrinkTime Row!");
		}
		else {
			GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + CurrentShrinkTime;
		}

		GameState->OnRep_SafeZoneIndicator();
		GameState->OnRep_SafeZonePhase();

		StartNewSafeZonePhaseOG(GameMode, ZoneIndex);
	}

	__int64 (*PickTeamOG)(AFortGameModeAthena* a1, unsigned __int8 a2, AFortPlayerControllerAthena* a3);
	__int64 __fastcall PickTeam(AFortGameModeAthena* a1, unsigned __int8 a2, AFortPlayerControllerAthena* a3)
	{
		int Ret = Globals::NextTeamIndex;

		++Globals::CurrentPlayersOnTeam;

		if (Globals::CurrentPlayersOnTeam == Globals::MaxPlayersPerTeam)
		{
			Globals::NextTeamIndex++;
			Globals::CurrentPlayersOnTeam = 0;
		}

		Log("PickTeam Called!");
		return Ret;
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x478A48C), ReadyToStartMatch, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x4793EFC), SpawnDefaultPawnFor, (LPVOID*)&SpawnDefaultPawnForOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x4796A0C), StartAircraftPhase, (LPVOID*)&StartAircraftPhaseOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x5294FE0), OnAircraftExitedDropZone, (LPVOID*)&OriginalOnAircraftExitedDropZone);

		MH_CreateHook((LPVOID)(ImageBase + 0x5294F44), OnAircraftEnteredDropZone, (LPVOID*)&OnAircraftEnteredDropZoneOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x4799688), StartNewSafeZonePhase, (LPVOID*)&StartNewSafeZonePhaseOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x4785400), PickTeam, (LPVOID*)&PickTeamOG);

		Log("FortGameModeAthena Hooked!");
	}
}