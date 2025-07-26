#pragma once
#include "framework.h"
#include "NpcSpawnLocations.h"

namespace BotsSpawner {
	void SpawnBosses() {
		FTransform Transform{};
		Transform.Translation = FVector();
		Transform.Rotation = FQuat();
		Transform.Scale3D = FVector{ 1,1,1 };

		int AmountSpawned = 0;

		auto SloneSpawnerData = StaticLoadObject<UClass>("/Slone/NPCs/Slone/Slone/BP_AIBotSpawnerData_Slone.BP_AIBotSpawnerData_Slone_C");
		auto SloneList = ((UFortAthenaAIBotSpawnerData*)SloneSpawnerData)->GetDefaultObj()->CreateComponentListFromClass(SloneSpawnerData, UWorld::GetWorld());
		Transform.Translation = NpcSpawnLocations::SloneSpawnLocations[rand() % (NpcSpawnLocations::SloneSpawnLocations.size())];
		((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AISpawner->RequestSpawn(SloneList, Transform);
		AmountSpawned++;

		Log("Spawned " + std::to_string(AmountSpawned) + " Bosses!");
	}

	void SpawnGuards() {
		int AmountSpawned = 0;

		Log("Spawned " + std::to_string(AmountSpawned) + " Guards!");
	}

	void SpawnNpcs() {
		int AmountSpawned = 0;

		Log("Spawned " + std::to_string(AmountSpawned) + " Npcs!");
	}

	void SpawnPlayerBot() {
		if (PlayerStarts.Num() == 0) {
			Log("No PlayerStarts!");
			return;
		}

		auto start = PlayerStarts[UKismetMathLibrary::RandomIntegerInRange(0, PlayerStarts.Num() - 1)];
		if (!start) {
			Log("No playerstart!");
			return;
		}
		FTransform Transform{};
		Transform.Translation = start->K2_GetActorLocation();
		Transform.Rotation = FQuat();
		Transform.Scale3D = FVector{ 1,1,1 };

		static auto PhoebeSpawnerData = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_AISpawnerData_Phoebe.BP_AISpawnerData_Phoebe_C");

		auto ComponentList = UFortAthenaAIBotSpawnerData::CreateComponentListFromClass(PhoebeSpawnerData, UWorld::GetWorld());

		((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AISpawner->RequestSpawn(ComponentList, Transform);
	}
}