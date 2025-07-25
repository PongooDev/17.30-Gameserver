#pragma once
#include "framework.h"

namespace Looting {
	char SpawnLoot(ABuildingContainer* BuildingContainer) {
		Log("SpawnLoot Called!");
		return true;
	}

	void SpawnFloorLoot()
	{
		auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

		TArray<AActor*> FloorLootSpawners;
		UClass* SpawnerClass = StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C");
		Statics->GetAllActorsOfClass(UWorld::GetWorld(), SpawnerClass, &FloorLootSpawners);

		for (size_t i = 0; i < FloorLootSpawners.Num(); i++)
		{
			FloorLootSpawners[i]->K2_DestroyActor();
		}

		FloorLootSpawners.Free();

		SpawnerClass = StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C");
		Statics->GetAllActorsOfClass(UWorld::GetWorld(), SpawnerClass, &FloorLootSpawners);

		for (size_t i = 0; i < FloorLootSpawners.Num(); i++)
		{
			FloorLootSpawners[i]->K2_DestroyActor();
		}

		FloorLootSpawners.Free();
	}

	void HookAll()
	{
		MH_CreateHook(reinterpret_cast<void*>(ImageBase + 0x498C198), SpawnLoot, nullptr);

		Log("Looting Hooked!");
	}
}