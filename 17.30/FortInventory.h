#pragma once
#include "framework.h"

namespace FortInventory {
	bool CompareGuids(FGuid Guid1, FGuid Guid2) {
		if (Guid1.A == Guid2.A
			&& Guid1.B == Guid2.B
			&& Guid1.C == Guid2.C
			&& Guid1.D == Guid2.D) {
			return true;
		}
		else {
			return false;
		}
	}

	void Update(AFortPlayerController* PC, FFortItemEntry* ItemEntry = nullptr)
	{
		PC->HandleWorldInventoryLocalUpdate();
		PC->WorldInventory->HandleInventoryLocalUpdate();
		PC->WorldInventory->bRequiresLocalUpdate = true;
		PC->WorldInventory->ForceNetUpdate();
		if (ItemEntry == nullptr)
			PC->WorldInventory->Inventory.MarkArrayDirty();
		else
			PC->WorldInventory->Inventory.MarkItemDirty(*ItemEntry);
	}

	void GiveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo, bool bShouldAddToExistingStack = false)
	{
		if (bShouldAddToExistingStack) {
			for (FFortItemEntry& ItemEntry : PC->WorldInventory->Inventory.ReplicatedEntries) {
				if (Def == ItemEntry.ItemDefinition) {
					ItemEntry.Count += Count;
					PC->WorldInventory->Inventory.MarkItemDirty(ItemEntry);
					Update(PC);
					break;
				}
			}
			return;
		}
		UFortWorldItem* Item = Cast<UFortWorldItem>(Def->CreateTemporaryItemInstanceBP(Count, 0));
		Item->SetOwningControllerForTemporaryItem(PC);
		Item->OwnerInventory = PC->WorldInventory;
		Item->ItemEntry.LoadedAmmo = LoadedAmmo;

		if (Item && Item->ItemEntry.ItemDefinition) {
			FFortItemEntryStateValue Value{};
			Value.IntValue = true;
			Value.StateType = EFortItemEntryState::ShouldShowItemToast;
			Item->ItemEntry.StateValues.Add(Value);
		}

		PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->WorldInventory->Inventory.ItemInstances.Add(Item);
		PC->WorldInventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->WorldInventory->HandleInventoryLocalUpdate();
	}

	void RemoveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count = INT_MAX) {
		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++) {
			FFortItemEntry& ItemEntry = PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (Def == ItemEntry.ItemDefinition) {
				ItemEntry.Count -= Count;
				if (ItemEntry.Count <= 0) {
					PC->WorldInventory->Inventory.ReplicatedEntries[i].StateValues.Free();
					PC->WorldInventory->Inventory.ReplicatedEntries.Remove(i);

					for (int i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
					{
						UFortWorldItem* WorldItem = PC->WorldInventory->Inventory.ItemInstances[i];
						if (WorldItem->ItemEntry.ItemDefinition == Def) {
							PC->WorldInventory->Inventory.ItemInstances.Remove(i);
						}
					}
				}
				else {
					Update(PC, &ItemEntry);
				}
				break;
			}
		}
		Update(PC);
	}

	FFortItemEntry* FindItemEntry(AFortPlayerControllerAthena* PC, UFortItemDefinition* ItemDef) {
		for (FFortItemEntry& ItemEntry : PC->WorldInventory->Inventory.ReplicatedEntries) {
			if (ItemDef == ItemEntry.ItemDefinition) {
				return &ItemEntry;
			}
		}

		return nullptr;
	}

	FFortItemEntry* FindItemEntry(AFortPlayerControllerAthena* PC, FGuid Guid) {
		for (FFortItemEntry& ItemEntry : PC->WorldInventory->Inventory.ReplicatedEntries) {
			if (CompareGuids(ItemEntry.ItemGuid, Guid)) {
				return &ItemEntry;
			}
		}

		return nullptr;
	}

	static EFortQuickBars GetQuickBars(UFortItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
			return EFortQuickBars::Primary;

		return EFortQuickBars::Secondary;
	}

	static bool IsInventoryFull(AFortPlayerController* PC)
	{
		if (!PC || !PC->WorldInventory) return true;

		static constexpr int Max = 5;
		int Count = 0;

		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry* Entries = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (!Entries) continue;
			if (!Entries->ItemDefinition) continue;

			if (GetQuickBars(Entries->ItemDefinition) == EFortQuickBars::Primary)
			{
				++Count;
				if (Count >= Max)
					return true;
			}
		}

		return false;
	}

	float GetMaxStackSize(UFortItemDefinition* Def)
	{
		if (!Def->MaxStackSize.Curve.CurveTable)
			return Def->MaxStackSize.Value;
		EEvaluateCurveTableResult Result;
		float Ret;
		((UDataTableFunctionLibrary*)UDataTableFunctionLibrary::StaticClass()->DefaultObject)->EvaluateCurveTableRow(Def->MaxStackSize.Curve.CurveTable, Def->MaxStackSize.Curve.RowName, 0, &Result, &Ret, FString());
		return Ret;
	}
}