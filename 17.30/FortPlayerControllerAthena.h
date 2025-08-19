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

		if (Command == "SpawnBot") {
			BotsSpawner::SpawnPlayerBot(Pawn);
		}
		else if (Command == "GodMode") {
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

	void ServerEndEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToStopEditing) {
		if (!BuildingActorToStopEditing || !PC->MyFortPawn || BuildingActorToStopEditing->bDestroyed == 1 || BuildingActorToStopEditing->EditingPlayer != PC->PlayerState)
			return;
		BuildingActorToStopEditing->SetNetDormancy(ENetDormancy::DORM_DormantAll);
		BuildingActorToStopEditing->EditingPlayer = nullptr;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			auto Item = PC->WorldInventory->Inventory.ItemInstances[i];
			if (Item->GetItemDefinitionBP()->IsA(UFortEditToolItemDefinition::StaticClass()))
			{
				PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->GetItemDefinitionBP(), Item->GetItemGuid(), Item->GetTrackerGuid(), false);
				break;
			}
		}
		if (!PC->MyFortPawn->CurrentWeapon || !PC->MyFortPawn->CurrentWeapon->WeaponData || !PC->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
			return;

		AFortWeap_EditingTool* EditTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
		EditTool->EditActor = nullptr;
		EditTool->OnRep_EditActor();
	}

	void ServerEditBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored) {
		if (!BuildingActorToEdit || BuildingActorToEdit->EditingPlayer != PC->PlayerState || !NewBuildingClass.Get() || BuildingActorToEdit->bDestroyed == 1)
			return;

		BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_DormantAll);
		BuildingActorToEdit->EditingPlayer = nullptr;
		ABuildingSMActor* EditedBuildingActor = ReplaceBuildingActor(BuildingActorToEdit, 1, NewBuildingClass.Get(), 0, RotationIterations, bMirrored, PC);
		if (EditedBuildingActor)
			EditedBuildingActor->bPlayerPlaced = true;
	}

	void ServerRepairBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToRepair) {
		auto FortKismet = (UFortKismetLibrary*)UFortKismetLibrary::StaticClass()->DefaultObject;
		if (!BuildingActorToRepair)
			return;

		if (BuildingActorToRepair->EditingPlayer)
		{
			return;
		}

		float BuildingHealthPercent = BuildingActorToRepair->GetHealthPercent();
		float BuildingCost = 10;
		float RepairCostMultiplier = 0.75;

		float BuildingHealthPercentLost = 1.0f - BuildingHealthPercent;
		float RepairCostUnrounded = (BuildingCost * BuildingHealthPercentLost) * RepairCostMultiplier;
		float RepairCost = std::floor(RepairCostUnrounded > 0 ? RepairCostUnrounded < 1 ? 1 : RepairCostUnrounded : 0);
		if (RepairCost < 0)
			return;

		auto ResourceDef = FortKismet->K2_GetResourceItemDefinition(BuildingActorToRepair->ResourceType);
		if (!ResourceDef)
			return;

		if (!PC->bBuildFree)
		{
			FortInventory::RemoveItem(PC, ResourceDef, (int)RepairCost);
		}

		BuildingActorToRepair->RepairBuilding(PC, (int)RepairCost);
	}

	void HookAll() {
		//MH_CreateHook((LPVOID)(ImageBase + 0xC264C0), ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x114, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x278, ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x215, ServerExecuteInventoryItem, (LPVOID*)&ServerExecuteInventoryItemOG);

		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x94, ServerAttemptAircraftJump, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x225, ServerAttemptInventoryDrop, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x1CF, ServerCheat, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x239, ServerCreateBuildingActor, (LPVOID*)&ServerCreateBuildingActorOG);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x240, ServerBeginEditingBuildingActor, (LPVOID*)&ServerBeginEditingBuildingActorOG);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x23E, ServerEndEditingBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x23B, ServerEditBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x235, ServerRepairBuildingActor, nullptr);

		Log("FortPlayerControllerAthena Hooked!");
	}
}