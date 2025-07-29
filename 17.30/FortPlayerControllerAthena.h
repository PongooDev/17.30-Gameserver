#pragma once
#include "framework.h"
#include "FortInventory.h"
#include "Looting.h"
#include "Vehicles.h"
#include "BotsSpawner.h"

namespace FortPlayerControllerAthena {
	void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* This, AFortPlayerPawnAthena* Pawn);
	void ServerAcknowledgePossession(AFortPlayerControllerAthena* This, AFortPlayerPawnAthena* Pawn) {
		Log("ServerAcknowledgePossession Called!");
		This->AcknowledgedPawn = Pawn;

		return ServerAcknowledgePossessionOG(This, Pawn);
	}

	void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
	void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC) {
		if (!PC) {
			Log("ServerReadyToStartMatch: No PC!");
			return;
		}

		static bool bSetupWorld = false;

		if (!bSetupWorld)
		{
			bSetupWorld = true;
			Looting::SpawnFloorLoot();
			Vehicles::SpawnVehicles();

			BotsSpawner::SpawnBosses();
			BotsSpawner::SpawnGuards();
			BotsSpawner::SpawnNpcs();

			Log("Setup World!");
		}

		return ServerReadyToStartMatchOG(PC);
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

	void ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PC, FGuid ItemGuid, int Count, bool bTrash)
	{
		FFortItemEntry* Entry = FortInventory::FindItemEntry(PC, ItemGuid);
		AFortPlayerPawn* Pawn = (AFortPlayerPawn*)PC->Pawn;
		SpawnPickup(Entry->ItemDefinition, Count, Entry->LoadedAmmo, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn);
		FortInventory::RemoveItem(PC, Entry->ItemDefinition, Count);
	}

	void ServerCheat(AFortPlayerControllerAthena* PC, FString& Msg) {
		if (Globals::bIsProdServer)
			return;

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
		auto Gamemode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)PC->Pawn;

		std::string Command = Msg.ToString();
		Log(Command);

		if (Command == "GodMode") {
			if (!PC->MyFortPawn->bIsInvulnerable) {
				PC->MyFortPawn->bIsInvulnerable = true;
			}
			else {
				PC->MyFortPawn->bIsInvulnerable = false;
			}
		}
		else if (Command == "DumpLoc") {
			FVector Loc = PC->Pawn->K2_GetActorLocation();
			Log("X: " + std::to_string(Loc.X));
			Log("Y: " + std::to_string(Loc.Y));
			Log("Z: " + std::to_string(Loc.Z));
		}
		else if (Command.contains("Teleport ")) {
			std::vector<std::string> args = TextManipUtils::SplitWhitespace(Command);
			FVector TeleportLoc = FVector();

			TeleportLoc.X = std::stoi(args[1]);
			TeleportLoc.Y = std::stoi(args[2]);
			TeleportLoc.Z = std::stoi(args[3]);

			if (!PC->Pawn->K2_TeleportTo(TeleportLoc, PC->Pawn->K2_GetActorRotation())) {
				FHitResult HitResult;
				Pawn->K2_SetActorLocation(TeleportLoc, false, &HitResult, true);
			}
			Log("Teleported: X: " + args[1] + " Y: " + args[2] + " Z: " + args[3]);
		}
		else if (Command == "TeleportToNPC") {
			FVector TeleportLoc = NpcAI::NpcBots[0]->Pawn->K2_GetActorLocation();
			if (!PC->Pawn->K2_TeleportTo(TeleportLoc, PC->Pawn->K2_GetActorRotation())) {
				FHitResult HitResult;
				Pawn->K2_SetActorLocation(TeleportLoc, false, &HitResult, true);
			}
		}
		else if (Command == "startaircraft")
		{
			UKismetSystemLibrary::GetDefaultObj()->ExecuteConsoleCommand(UWorld::GetWorld(), TEXT("startaircraft"), nullptr);
		}
		else if (Command == "pausesafezone")
		{
			UKismetSystemLibrary::GetDefaultObj()->ExecuteConsoleCommand(UWorld::GetWorld(), TEXT("pausesafezone"), nullptr);
		}
	}

	void HookAll() {
		//MH_CreateHook((LPVOID)(ImageBase + 0xC264C0), ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x114, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x278, ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x215, ServerExecuteInventoryItem, (LPVOID*)&ServerExecuteInventoryItemOG);

		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x94, ServerAttemptAircraftJump, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x225, ServerAttemptInventoryDrop, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x1CF, ServerCheat, nullptr);

		Log("FortPlayerControllerAthena Hooked!");
	}
}