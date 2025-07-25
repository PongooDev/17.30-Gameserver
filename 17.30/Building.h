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
		FortInventory::Update(PC);

		return OnDamageServerOG(This, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x515FEA4), OnDamageServer, (LPVOID*)&OnDamageServerOG);

		Log("Building Hooked!");
	}
}