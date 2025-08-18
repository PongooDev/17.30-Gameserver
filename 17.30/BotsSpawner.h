#pragma once
#include "framework.h"
#include "FortAthenaAIBotController.h"

namespace BotsSpawner {
	TArray<FVector> GetSpawnLocations(std::string Name)
	{
		TArray<FVector> SpawnLocations;

		AFortAthenaPatrolPath* PatrolPath = nullptr;
		for (auto& Path : GetAllActorsOfClass<AFortAthenaPatrolPathPointProvider>())
		{
			if (Path->FiltersTags.GameplayTags.Num() == 0)
				continue;
			auto PathName = Path->FiltersTags.GameplayTags[0].TagName.ToString();
			if (PathName.contains(Name) || Path->Name.ToString().contains(Name)) {
				PatrolPath = Path->AssociatedPatrolPath;
				break;
			}
		}
		if (!PatrolPath) {
			for (auto& Path : GetAllActorsOfClass<AFortAthenaPatrolPath>())
			{
				if (Path->Name.ToString().contains(Name)) {
					PatrolPath = Path;
					break;
				}
			}
		}

		if (PatrolPath && PatrolPath->PatrolPoints.Num() > 0)
		{
			for (int i = 0; i < PatrolPath->PatrolPoints.Num(); i++) {
				SpawnLocations.Add(PatrolPath->PatrolPoints[i]->K2_GetActorLocation());
			}
		}
		else
		{
			Log("No Paths Found For Name: " + Name);
		}

		return SpawnLocations;
	}

	bool SpawnBot(std::string Name, UClass* SpawnerData) {
		FortAthenaAIBotController::BotSpawnData BotSpawnData;

		FTransform Transform{};
		Transform.Translation = FVector();
		Transform.Rotation = FQuat();
		Transform.Scale3D = FVector{ 1,1,1 };

		TArray<FVector> Locs = GetSpawnLocations(Name);
		if (Locs.Num() > 0) {
			auto List = ((UFortAthenaAIBotSpawnerData*)SpawnerData)->CreateComponentListFromClass(SpawnerData, UWorld::GetWorld());

			Transform.Translation = Locs[rand() % (Locs.Num())];

			int32 RequestID = ((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AISpawner->RequestSpawn(List, Transform);
			BotSpawnData.RequestID = RequestID;
			BotSpawnData.BotSpawnerData = SpawnerData;
			BotSpawnData.Name = Name;
			FortAthenaAIBotController::SpawnedBots.push_back(BotSpawnData);
			return true;
		}
		else {
			return false;
		}
	}

	void SpawnBosses() {
		static std::vector<std::pair<std::string, UClass*>> Bosses = {
			{
				"Slone",
				StaticLoadObject<UClass>("/Slone/NPCs/Slone/Slone/BP_AIBotSpawnerData_Slone.BP_AIBotSpawnerData_Slone_C")
			}
		};
		int AmountSpawned = 0;

		for (auto& [Name, SpawnerData] : Bosses)
		{
			if (SpawnBot(Name, SpawnerData)) {
				AmountSpawned++;
			}
		}

		Log("Spawned " + std::to_string(AmountSpawned) + " Bosses!");
	}

	void SpawnGuards() {
		static std::vector<std::pair<std::string, UClass*>> Guards = {
			{
				"IO_Phase1_10",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			},
			{
				"IO_Phase1_2",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			},
			{
				"IO_Phase1_4",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			},
			{
				"IO_Phase1_5",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			},
			{
				"IO_Phase1_7",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			},
			{
				"IO_Phase1_8",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			},
			{
				"IO_Phase1__9",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			},
			{
				"IO_Small_Phase1_1",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			},
			{
				"IO_Small_Phase1_2",
				StaticLoadObject<UClass>("/IO_Guard/AI/NPCs/IO_Compound/BP_AIBotSpawnerData_IO_Compound.BP_AIBotSpawnerData_IO_Compound_C")
			}
		};
		int AmountSpawned = 0;

		for (auto& [Name, SpawnerData] : Guards)
		{
			if (SpawnBot(Name, SpawnerData)) {
				AmountSpawned++;
			}
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