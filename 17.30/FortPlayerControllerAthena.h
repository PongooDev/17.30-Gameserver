#pragma once
#include "framework.h"
#include "FortInventory.h"

namespace FortPlayerControllerAthena {
	void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* This, AFortPlayerPawnAthena* Pawn);
	void ServerAcknowledgePossession(AFortPlayerControllerAthena* This, AFortPlayerPawnAthena* Pawn) {
		Log("ServerAcknowledgePossession Called!");
		This->AcknowledgedPawn = Pawn;

		return ServerAcknowledgePossessionOG(This, Pawn);
	}

	void (*ServerExecuteInventoryItemOG)(AFortPlayerControllerAthena* PC, FGuid& ItemGuid);
	void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PC, FGuid& ItemGuid) {
		if (!PC || PC->IsInAircraft()) {
			return ServerExecuteInventoryItemOG(PC, ItemGuid);
		}
		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)PC->Pawn;

		FFortItemEntry* ItemEntry = FortInventory::FindItemEntry(PC, ItemGuid);
		if (ItemEntry) {
			Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)ItemEntry->ItemDefinition, ItemEntry->ItemGuid, ItemEntry->TrackerGuid, false);
		}

		return ServerExecuteInventoryItemOG(PC, ItemGuid);
	}

	void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* Comp, FRotator ClientRotation)
	{
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		auto PC = (AFortPlayerControllerAthena*)Comp->GetOwner();
		UWorld::GetWorld()->AuthorityGameMode->RestartPlayer(PC);

		if (PC->MyFortPawn)
		{
			PC->ClientSetRotation(ClientRotation, true);
			PC->MyFortPawn->BeginSkydiving(true);
			PC->MyFortPawn->SetHealth(100);
			PC->MyFortPawn->SetShield(0);
		}
	}

	void HookAll() {
		//MH_CreateHook((LPVOID)(ImageBase + 0xC264C0), ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x114, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x215, ServerExecuteInventoryItem, (LPVOID*)&ServerExecuteInventoryItemOG);

		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x94, ServerAttemptAircraftJump, nullptr);

		Log("FortPlayerControllerAthena Hooked!");
	}
}