#pragma once
#include "framework.h"
#include "FortInventory.h"

namespace Building {
	void (*OnDamageServerOG)(ABuildingActor* This, float Damage, FGameplayTagContainer& DamageTags, FVector& Momentum, FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle& EffectContext);
	void OnDamageServer(ABuildingActor* This, float Damage, FGameplayTagContainer& DamageTags, FVector& Momentum, FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle& EffectContext) {
		if (!This || !InstigatedBy || !InstigatedBy->IsA(AFortPlayerControllerAthena::StaticClass()) || !DamageCauser->IsA(AFortWeapon::StaticClass()) || !((AFortWeapon*)DamageCauser)->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || This->bPlayerPlaced) {
			return OnDamageServerOG(This, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
		}
		
		ABuildingSMActor* BuildingSMActor = (ABuildingSMActor*)This;
		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)InstigatedBy;
		if (!PC->Pawn) {
			return OnDamageServerOG(This, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
		}

		int MaterialCount = (Damage / (UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(4, 8)));

		PC->ClientReportDamagedResourceBuilding(BuildingSMActor, BuildingSMActor->ResourceType, MaterialCount, BuildingSMActor->bDestroyed, (Damage == 100.f));

		UFortResourceItemDefinition* ResourceItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->ResourceType);
		if (!ResourceItemDefinition) {
			return OnDamageServerOG(This, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
		}
		FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PC, ResourceItemDefinition);
		if (!ItemEntry) {
			FortInventory::GiveItem(PC, ResourceItemDefinition, MaterialCount, 0);
		}
		else {
			int MaxStackSize = ItemEntry->ItemDefinition->MaxStackSize.Value;
			int Count = ItemEntry->Count;

			if (Count >= MaxStackSize)
			{
				//Log("Full!");
				SpawnPickup(ItemEntry->ItemDefinition, MaterialCount, ItemEntry->LoadedAmmo, PC->MyFortPawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, PC->MyFortPawn);
			}
			else
			{
				int Space = MaxStackSize - Count;
				int AddToStack = UKismetMathLibrary::GetDefaultObj()->Min(Space, MaterialCount);
				int LeftOver = MaterialCount - AddToStack;

				//Log("AddToStack: " + std::to_string(AddToStack));
				//Log("Count: " + std::to_string(Count));
				FortInventory::GiveItem(PC, ResourceItemDefinition, AddToStack, 0, true);

				if (LeftOver > 0) {
					SpawnPickup(ItemEntry->ItemDefinition, LeftOver, ItemEntry->LoadedAmmo, PC->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, true, PC->MyFortPawn);
				}
			}
		}

		return OnDamageServerOG(This, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}

	void (*ServerCreateBuildingActorOG)(AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData);
	void ServerCreateBuildingActor(AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData) {
		Log("ServerCreateBuildingActor Called!");
		if (!PC) {
			Log("No PC!");
			return;
		}

		UClass* BuildingClass = PC->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();

		TArray<ABuildingSMActor*> BuildingsToRemove;
		char BuildRestrictionFlag;
		if (CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &BuildingsToRemove, &BuildRestrictionFlag))
		{
			Log("CantBuild!");
			return; 
		}

		auto ResourceItemDefinition = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType);
		FortInventory::RemoveItem(PC, ResourceItemDefinition, 10);

		ABuildingSMActor* PlacedBuilding = SpawnActor<ABuildingSMActor>(CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PC, BuildingClass);
		PlacedBuilding->bPlayerPlaced = true;
		PlacedBuilding->InitializeKismetSpawnedBuildingActor(PlacedBuilding, PC, true, nullptr);
		PlacedBuilding->TeamIndex = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex;
		PlacedBuilding->Team = EFortTeam(PlacedBuilding->TeamIndex);

		for (size_t i = 0; i < BuildingsToRemove.Num(); i++)
		{
			BuildingsToRemove[i]->K2_DestroyActor();
		}
		BuildingsToRemove.Free();
	}

	void (*ServerBeginEditingBuildingActorOG)(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit);
	void ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit)
	{
		//Log("ServerBeginEditingBuildingActor Called!");
		if (!BuildingActorToEdit || !BuildingActorToEdit->bPlayerPlaced || !PC->MyFortPawn)
			return;

		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_Awake);
		BuildingActorToEdit->EditingPlayer = PlayerState;

		for (int i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			auto Item = PC->WorldInventory->Inventory.ItemInstances[i];
			if (Item->GetItemDefinitionBP()->IsA(UFortEditToolItemDefinition::StaticClass()))
			{
				PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->GetItemDefinitionBP(), Item->GetItemGuid(), Item->GetTrackerGuid(), false);
				break;
			}
		}

		if (!PC->MyFortPawn->CurrentWeapon || !PC->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
			return;

		AFortWeap_EditingTool* EditTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
		EditTool->EditActor = BuildingActorToEdit;
		EditTool->OnRep_EditActor();

		return ServerBeginEditingBuildingActorOG(PC, BuildingActorToEdit);
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x515FEA4), OnDamageServer, (LPVOID*)&OnDamageServerOG);

		//MH_CreateHook((LPVOID)(ImageBase + 0x53951E0), ServerCreateBuildingActor, (LPVOID*)&ServerCreateBuildingActorOG);
		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x239, ServerCreateBuildingActor, (LPVOID*)&ServerCreateBuildingActorOG);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x240, ServerBeginEditingBuildingActor, (LPVOID*)&ServerBeginEditingBuildingActorOG);

		Log("Building Hooked!");
	}
}