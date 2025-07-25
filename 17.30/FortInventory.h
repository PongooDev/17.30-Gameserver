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

	inline void GiveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo, bool bShouldAddToExistingStack = false)
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

		PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->WorldInventory->Inventory.ItemInstances.Add(Item);
		PC->WorldInventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->WorldInventory->HandleInventoryLocalUpdate();
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
}