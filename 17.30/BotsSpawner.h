#pragma once
#include "framework.h"
#include "FortAthenaAIBotController.h"
#include "NpcSpawnLocations.h"

namespace BotsSpawner {
	void SpawnBosses() {
		FortAthenaAIBotController::BotSpawnData BotSpawnData;
		int32 RequestID = 0;

		FTransform Transform{};
		Transform.Translation = FVector();
		Transform.Rotation = FQuat();
		Transform.Scale3D = FVector{ 1,1,1 };

		int AmountSpawned = 0;

		UClass* SloneSpawnerData = StaticLoadObject<UClass>("/Slone/NPCs/Slone/Slone/BP_AIBotSpawnerData_Slone.BP_AIBotSpawnerData_Slone_C");
		Transform.Translation = NpcSpawnLocations::SloneSpawnLocations[rand() % (NpcSpawnLocations::SloneSpawnLocations.size())];
		UFortAthenaAISpawnerDataComponentList* SloneList = ((UFortAthenaAIBotSpawnerData*)SloneSpawnerData)->GetDefaultObj()->CreateComponentListFromClass(SloneSpawnerData, UWorld::GetWorld());
		RequestID = ((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AISpawner->RequestSpawn(SloneList, Transform);
		BotSpawnData.RequestID = RequestID;
		BotSpawnData.BotSpawnerData = SloneSpawnerData;
		FortAthenaAIBotController::SpawnedBots.push_back(BotSpawnData);
		AmountSpawned++;

		Log("Spawned " + std::to_string(AmountSpawned) + " Bosses!");
	}

	void SpawnGuards() {
		FortAthenaAIBotController::BotSpawnData BotSpawnData;
		int32 RequestID = 0;

		FTransform Transform{};
		Transform.Translation = FVector();
		Transform.Rotation = FQuat();
		Transform.Scale3D = FVector{ 1,1,1 };

		int AmountSpawned = 0;

		UClass* IOGruntSpawnerData = StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C");
		UFortAthenaAISpawnerDataComponentList* IOGruntList = ((UFortAthenaAIBotSpawnerData*)IOGruntSpawnerData)->GetDefaultObj()->CreateComponentListFromClass(IOGruntSpawnerData, UWorld::GetWorld());
		for (int i = 0; i < NpcSpawnLocations::IOGruntSpawnLocations.size(); i++) {
			Transform.Translation = NpcSpawnLocations::IOGruntSpawnLocations[i];
			RequestID = ((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AISpawner->RequestSpawn(IOGruntList, Transform);
			BotSpawnData.RequestID = RequestID;
			BotSpawnData.BotSpawnerData = IOGruntSpawnerData;
			FortAthenaAIBotController::SpawnedBots.push_back(BotSpawnData);
			AmountSpawned++;
		}

		Log("Spawned " + std::to_string(AmountSpawned) + " Guards!");
	}

	void SpawnNpcs() {
		int AmountSpawned = 0;

		Log("Spawned " + std::to_string(AmountSpawned) + " Npcs!");
	}

	void SpawnPlayerBot() {
		if (PlayerStarts.Num() == 0) {
			Log("No PlayerStarts!");
			UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);
			return;
		}

		auto start = PlayerStarts[UKismetMathLibrary::RandomIntegerInRange(0, PlayerStarts.Num() - 1)];
		if (!start) {
			Log("No playerstart!");
			return;
		}
		FortAthenaAIBotController::BotSpawnData BotSpawnData;

		FTransform Transform{};
		Transform.Translation = start->K2_GetActorLocation();
		Transform.Rotation = FQuat();
		Transform.Scale3D = FVector{ 1,1,1 };

		static auto PhoebeSpawnerData = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_AISpawnerData_Phoebe.BP_AISpawnerData_Phoebe_C");
		auto ComponentList = UFortAthenaAIBotSpawnerData::CreateComponentListFromClass(PhoebeSpawnerData, UWorld::GetWorld());

		int32 RequestID = ((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AISpawner->RequestSpawn(ComponentList, Transform);
		BotSpawnData.RequestID = RequestID;
		BotSpawnData.BotSpawnerData = PhoebeSpawnerData;
		FortAthenaAIBotController::SpawnedBots.push_back(BotSpawnData);
	}
}