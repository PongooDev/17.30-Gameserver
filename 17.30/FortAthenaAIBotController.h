#pragma once
#include "framework.h"
#include "Looting.h"
#include "AbilitySystemComponent.h"
#include "NpcAI.h"

#include "PhoebeDisplayNames.h"

namespace FortAthenaAIBotController {
	struct BotSpawnData {
		UClass* BotSpawnerData;
		int32 RequestID;
		FString BotIDSuffix;
		std::string Name;

		AFortAthenaAIBotController* Controller;
		AFortPlayerPawnAthena* Pawn;
		AFortPlayerStateAthena* PlayerState;
	};
	std::vector<BotSpawnData> SpawnedBots;

	void (*CreateAndConfigureNavigationSystemOG)(UAthenaNavSystemConfig* ModuleConfig, UWorld* World);
	void CreateAndConfigureNavigationSystem(UAthenaNavSystemConfig* ModuleConfig, UWorld* World)
	{
		Log("CreateAndConfigureNavigationSystem For World: " + World->GetName() + " For NavConfig: " + ModuleConfig->GetName());
		ModuleConfig->bPrioritizeNavigationAroundSpawners = true;
		ModuleConfig->bAutoSpawnMissingNavData = true;
		ModuleConfig->bSpawnNavDataInNavBoundsLevel = true;
		return CreateAndConfigureNavigationSystemOG(ModuleConfig, World);
	}

	// Pathfinding
	void (*InitializeForWorldOG)(UNavigationSystemV1* NavSystem, UWorld* World, EFNavigationSystemRunMode Mode);
	void InitializeForWorld(UAthenaNavSystem* NavSystem, UWorld* World, EFNavigationSystemRunMode Mode)
	{
		Log("InitialiseForWorld: " + World->GetName() + " For NavSystem: " + NavSystem->GetName());
		NavSystem->bAutoCreateNavigationData = true;
		AthenaNavSystem = NavSystem;
		return InitializeForWorldOG(NavSystem, World, Mode);
	}

	void (*OnPawnAISpawnedOG)(AActor* Controller, AFortPlayerPawnAthena* Pawn);
	void OnPawnAISpawned(AActor* Controller, AFortPlayerPawnAthena* Pawn)
	{
		static int AmountTimesCalled = 0;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		OnPawnAISpawnedOG(Controller, Pawn);
		if (!AthenaNavSystem->MainNavData) {
			Log("NavData Dont Exist!");
		}

		std::string BotName = "";

		UClass* BotSpawnerData = nullptr;
		auto PC = (AFortAthenaAIBotController*)Pawn->Controller;
		auto PlayerState = (AFortPlayerStateAthena*)Pawn->PlayerState;
		for (BotSpawnData& SpawnedBot : SpawnedBots) {
			if (AmountTimesCalled == SpawnedBot.RequestID) {
				SpawnedBot.BotIDSuffix = PC->BotIDSuffix;
				SpawnedBot.Controller = PC;
				SpawnedBot.Pawn = Pawn;
				SpawnedBot.PlayerState = PlayerState;
				BotName = SpawnedBot.Name;
				if (SpawnedBot.BotSpawnerData) {
					BotSpawnerData = SpawnedBot.BotSpawnerData;
				}
			}
		}
		if (PC->BotIDSuffix.ToString().contains("Clone")) {
			Log("SloneClone");
			BotSpawnerData = StaticLoadObject<UClass>("/Slone/NPCs/Slone/SloneClone/BP_AIBotSpawnerData_SloneClone.BP_AIBotSpawnerData_SloneClone_C");
		}
		if (BotSpawnerData) {
			UFortAthenaAIBotSpawnerData* SpawnerData = Cast<UFortAthenaAIBotSpawnerData>(BotSpawnerData->DefaultObject);
			if (!SpawnerData) {
				Log("No SpawnerData!");
				return;
			}

			UFortAthenaAISpawnerDataComponent_AIBotInventory* InventoryComponent = (UFortAthenaAISpawnerDataComponent_AIBotInventory*)SpawnerData->GetInventoryComponent();
			if (!PC->StartupInventory) {
				PC->StartupInventory = (UFortAthenaAIBotInventoryItems*)UGameplayStatics::GetDefaultObj()->SpawnObject(UFortAthenaAIBotInventoryItems::StaticClass(), GameMode);
			}
			if (InventoryComponent) {
				PC->StartupInventory->Items = InventoryComponent->Items;
			}

			UFortAthenaAISpawnerDataComponent_SkillsetBase* SkillSetBase = SpawnerData->GetSkillSetComponent();
			if (SkillSetBase) {
				PC->BotSkillSetClasses = SkillSetBase->GetSkillSets();
			}

			UFortAthenaAISpawnerDataComponent_ConversationBase* ConversationComp = SpawnerData->GetConversationComponent();

			AbilitySystemComponent::GiveAbilitySet(StaticLoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer"), PlayerState);
		}

		if (!PC->PathFollowingComponent->MyNavData) {
			PC->PathFollowingComponent->MyNavData = AthenaNavSystem->MainNavData;
		}
		PC->PathFollowingComponent->OnNavDataRegistered(PC->PathFollowingComponent->MyNavData);
		PC->PathFollowingComponent->Activate(true);

		if (PC->CachedAIServicePlayerBots) {
			
		}
		else {
			Log("No CachedAIServicePlayerBots!");
		}

		if (Pawn->Controller->Class == ABP_PhoebePlayerController_C::StaticClass())
		{
			ABP_PhoebePlayerController_C* BotPC = (ABP_PhoebePlayerController_C*)PC;
			if (BotPC)
			{
				if (!PC->RunBehaviorTree(PC->BehaviorTree)) {
					Log("BehaviorTree Failed To Run For Pawn: " + Pawn->GetFullName());
				}
				else {
					Log("Ran BehaviorTree: " + PC->BehaviorTree->GetFullName());
					BotPC->BlueprintOnBehaviorTreeStarted();
				}

				BotPC->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_Global_GamePhaseStep"), (int)GameState->GamePhaseStep);
				BotPC->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_Global_GamePhase"), (int)GameState->GamePhase);
				BotPC->Blackboard->SetValueAsBool(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_Global_HasEverJumpedFromBusKey"), false);
				BotPC->Blackboard->SetValueAsBool(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_Global_IsMovementBlocked"), false);

				if (UKismetMathLibrary::RandomBool()) {
					BotPC->Blackboard->SetValueAsBool(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_WarmupPlayEmote_ExecutionStatus"), (int)EExecutionStatus::ExecutionAllowed);
				}
				else {
					if (UKismetMathLibrary::RandomBool()) {
						BotPC->Blackboard->SetValueAsBool(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_WarmupLootAndShoot_ExecutionStatus"), (int)EExecutionStatus::ExecutionAllowed);
					}
				}

				if (BuildingFoundations.Num() > 0) {
					AActor* DropZone = BuildingFoundations[UKismetMathLibrary::RandomIntegerInRange(0, BuildingFoundations.Num() - 1)];
					if (DropZone) {
						BotPC->Blackboard->SetValueAsVector(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_JumpOffBus_Destination"), DropZone->K2_GetActorLocation());
					}
				}
				else {
					Log("No building foundations!");
				}

				auto BotPlayerState = (AFortPlayerStateAthena*)Pawn->PlayerState;
				if (!Characters.empty()) {
					auto CID = Characters[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Characters.size() - 1)];
					if (CID->HeroDefinition)
					{
						if (CID->HeroDefinition->Specializations.IsValid())
						{
							for (size_t i = 0; i < CID->HeroDefinition->Specializations.Num(); i++)
							{
								UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(CID->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());
								if (Spec)
								{
									for (size_t j = 0; j < Spec->CharacterParts.Num(); j++)
									{
										UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName).ToString());
										if (Part)
										{
											BotPlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
										}
									}
								}
							}
						}
					}
					if (CID) {
						Pawn->CosmeticLoadout.Character = CID;
					}
				}
				if (!Backpacks.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5)) { // less likely to equip than skin cause lots of ppl prefer not to use backpack
					auto Backpack = Backpacks[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Backpacks.size() - 1)];
					for (size_t j = 0; j < Backpack->CharacterParts.Num(); j++)
					{
						UCustomCharacterPart* Part = Backpack->CharacterParts[j];
						if (Part)
						{
							BotPlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
						}
					}
				}
				if (!Gliders.empty()) {
					auto Glider = Gliders[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Gliders.size() - 1)];
					Pawn->CosmeticLoadout.Glider = Glider;
				}
				if (!Contrails.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.95)) {
					auto Contrail = Contrails[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Contrails.size() - 1)];
					Pawn->CosmeticLoadout.SkyDiveContrail = Contrail;
				}
				for (size_t i = 0; i < Dances.size(); i++)
				{
					Pawn->CosmeticLoadout.Dances.Add(Dances.at(i));
				}
				BotPlayerState->OnRep_CharacterData();

				if (PhoebeDisplayNames.size() != 0) {
					std::srand(static_cast<unsigned int>(std::time(0)));
					int randomIndex = std::rand() % PhoebeDisplayNames.size();
					std::string rdName = PhoebeDisplayNames[randomIndex];
					PhoebeDisplayNames.erase(PhoebeDisplayNames.begin() + randomIndex);

					int size_needed = MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), NULL, 0);
					std::wstring wideString(size_needed, 0);
					MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), &wideString[0], size_needed);


					FString CVName = FString(wideString.c_str());
					GameMode->ChangeName(BotPC, CVName, true);

					BotPlayerState->OnRep_PlayerName();
				}

				for (auto SkillSet : BotPC->BotSkillSetClasses)
				{
					if (!SkillSet)
						continue;

					if (auto AimingSkill = Cast<UFortAthenaAIBotAimingDigestedSkillSet>(SkillSet))
						BotPC->CacheAimingDigestedSkillSet = AimingSkill;

					if (auto AttackingSkill = Cast<UFortAthenaAIBotAttackingDigestedSkillSet>(SkillSet))
						BotPC->CacheAttackingSkillSet = AttackingSkill;

					if (auto HarvestSkill = Cast<UFortAthenaAIBotHarvestDigestedSkillSet>(SkillSet))
						BotPC->CacheHarvestDigestedSkillSet = HarvestSkill;

					if (auto InventorySkill = Cast<UFortAthenaAIBotInventoryDigestedSkillSet>(SkillSet))
						BotPC->CacheInventoryDigestedSkillSet = InventorySkill;

					if (auto LootingSkill = Cast<UFortAthenaAIBotLootingDigestedSkillSet>(SkillSet))
						BotPC->CacheLootingSkillSet = LootingSkill;

					if (auto MovementSkill = Cast<UFortAthenaAIBotMovementDigestedSkillSet>(SkillSet))
						BotPC->CacheMovementSkillSet = MovementSkill;

					if (auto PerceptionSkill = Cast<UFortAthenaAIBotPerceptionDigestedSkillSet>(SkillSet))
						BotPC->CachePerceptionDigestedSkillSet = PerceptionSkill;

					if (auto PlayStyleSkill = Cast<UFortAthenaAIBotPlayStyleDigestedSkillSet>(SkillSet))
						BotPC->CachePlayStyleSkillSet = PlayStyleSkill;

					if (auto RangeAttackSkill = Cast<UFortAthenaAIBotRangeAttackDigestedSkillSet>(SkillSet))
						BotPC->CacheRangeAttackSkillSet = RangeAttackSkill;

					if (auto UnstuckSkill = Cast<UFortAthenaAIBotUnstuckDigestedSkillSet>(SkillSet))
						BotPC->CacheUnstuckSkillSet = UnstuckSkill;
				}

				if (!BotPC->Inventory)
					BotPC->Inventory = SpawnActor<AFortInventory>({}, {}, BotPC);

				for (auto& Items : ((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->StartingItems)
				{
					if (!Items.Item)
						continue;
					UFortWorldItem* Item = Cast<UFortWorldItem>(Items.Item->CreateTemporaryItemInstanceBP(Items.Count, 0));
					Item->OwnerInventory = BotPC->Inventory;
					FFortItemEntry& Entry = Item->ItemEntry;
					BotPC->Inventory->Inventory.ReplicatedEntries.Add(Entry);
					BotPC->Inventory->Inventory.ItemInstances.Add(Item);
					BotPC->Inventory->Inventory.MarkItemDirty(Entry);
					BotPC->Inventory->HandleInventoryLocalUpdate();
				}

				for (auto& Items : BotPC->StartupInventory->Items)
				{
					if (!Items.Item)
						continue;
					UFortWorldItem* Item = Cast<UFortWorldItem>(Items.Item->CreateTemporaryItemInstanceBP(Items.Count, 0));
					Item->OwnerInventory = BotPC->Inventory;
					FFortItemEntry& Entry = Item->ItemEntry;
					Entry.LoadedAmmo = 1;
					BotPC->Inventory->Inventory.ReplicatedEntries.Add(Entry);
					BotPC->Inventory->Inventory.ItemInstances.Add(Item);
					BotPC->Inventory->Inventory.MarkItemDirty(Entry);
					BotPC->Inventory->HandleInventoryLocalUpdate();
					if (auto WeaponDef = Cast<UFortWeaponMeleeItemDefinition>(Entry.ItemDefinition))
					{
						BotPC->PendingEquipWeapon = Item;
						Pawn->EquipWeaponDefinition(WeaponDef, Entry.ItemGuid, Entry.TrackerGuid, false);
					}
				}

				GameMode->AliveBots.Add(BotPC);
				GameState->PlayerBotsLeft++;
				GameState->OnRep_PlayerBotsLeft();
			}
			AmountTimesCalled++;
			return;
		}

		for (auto SkillSet : PC->BotSkillSetClasses)
		{
			if (!SkillSet)
				continue;

			if (auto AimingSkill = Cast<UFortAthenaAIBotAimingDigestedSkillSet>(SkillSet))
				PC->CacheAimingDigestedSkillSet = AimingSkill;

			if (auto AttackingSkill = Cast<UFortAthenaAIBotAttackingDigestedSkillSet>(SkillSet))
				PC->CacheAttackingSkillSet = AttackingSkill;

			if (auto HarvestSkill = Cast<UFortAthenaAIBotHarvestDigestedSkillSet>(SkillSet))
				PC->CacheHarvestDigestedSkillSet = HarvestSkill;

			if (auto InventorySkill = Cast<UFortAthenaAIBotInventoryDigestedSkillSet>(SkillSet))
				PC->CacheInventoryDigestedSkillSet = InventorySkill;

			if (auto LootingSkill = Cast<UFortAthenaAIBotLootingDigestedSkillSet>(SkillSet))
				PC->CacheLootingSkillSet = LootingSkill;

			if (auto MovementSkill = Cast<UFortAthenaAIBotMovementDigestedSkillSet>(SkillSet))
				PC->CacheMovementSkillSet = MovementSkill;

			if (auto PerceptionSkill = Cast<UFortAthenaAIBotPerceptionDigestedSkillSet>(SkillSet))
				PC->CachePerceptionDigestedSkillSet = PerceptionSkill;

			if (auto PlayStyleSkill = Cast<UFortAthenaAIBotPlayStyleDigestedSkillSet>(SkillSet))
				PC->CachePlayStyleSkillSet = PlayStyleSkill;

			if (auto RangeAttackSkill = Cast<UFortAthenaAIBotRangeAttackDigestedSkillSet>(SkillSet))
				PC->CacheRangeAttackSkillSet = RangeAttackSkill;

			if (auto UnstuckSkill = Cast<UFortAthenaAIBotUnstuckDigestedSkillSet>(SkillSet))
				PC->CacheUnstuckSkillSet = UnstuckSkill;
		}

		ApplyCharacterCustomization(PlayerState, Pawn);

		if (Globals::bBotsShouldUseManualTicking) {
			PC->BrainComponent->StopLogic(L"Manual Ticking Enabled!");
		}
		PC->Blackboard->SetValueAsEnum(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_GamePhaseStep"), (int)GameState->GamePhaseStep);
		PC->Blackboard->SetValueAsEnum(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_GamePhase"), (int)GameState->GamePhase);
		PC->Blackboard->SetValueAsBool(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_IsMovementBlocked"), false);
		PC->Blackboard->SetValueAsEnum(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_RangeAttack_ExecutionStatus"), (int)EExecutionStatus::ExecutionAllowed);

		if (PC->StartupInventory) {
			for (auto& Items : PC->StartupInventory->Items)
			{
				UFortItemDefinition* ItemDef = Items.Item;
				if (!ItemDef) {
					continue;
				}

				UFortWorldItem* Item = (UFortWorldItem*)ItemDef->CreateTemporaryItemInstanceBP(Items.Count, 0);
				Item->OwnerInventory = PC->Inventory;
				Item->ItemEntry.LoadedAmmo = 60;
				PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
				PC->Inventory->Inventory.ItemInstances.Add(Item);
				PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
				PC->Inventory->HandleInventoryLocalUpdate();
				if (auto WeaponDef = Cast<UFortWeaponRangedItemDefinition>(Item->ItemEntry.ItemDefinition))
				{
					PC->PendingEquipWeapon = Item;
					Pawn->EquipWeaponDefinition(WeaponDef, Item->ItemEntry.ItemGuid, Item->ItemEntry.TrackerGuid, false);
				}
			}
		}
		else {
			Log("StartupInventory is nullptr!");
		}

		bool bSetupPatrollingComp = false;
		if (!bSetupPatrollingComp) {
			for (AFortAthenaPatrolPathPointProvider* PatrolPointProvider : GetAllActorsOfClass<AFortAthenaPatrolPathPointProvider>()) {
				if (PatrolPointProvider->AssociatedPatrolPath && (PatrolPointProvider->Name.ToString().contains(BotName.empty() ? PC->BotIDSuffix.ToString() : BotName) || PatrolPointProvider->AssociatedPatrolPath->GetFullName().contains(BotName.empty() ? PC->BotIDSuffix.ToString() : BotName))) {
					Log("Found Patrol Path For Name: " + (BotName.empty() ? PC->BotIDSuffix.ToString() : BotName));
					PC->CachedPatrollingComponent->SetPatrolPath(PatrolPointProvider->AssociatedPatrolPath);
					bSetupPatrollingComp = true;
					break;
				}
			}

			if (!bSetupPatrollingComp) {
				for (AFortAthenaPatrolPath* PatrolPath : GetAllActorsOfClass<AFortAthenaPatrolPath>()) {
					if (PatrolPath && PatrolPath->Name.ToString().contains(BotName.empty() ? PC->BotIDSuffix.ToString() : BotName)) {
						Log("Found Patrol Path For Name: " + (BotName.empty() ? PC->BotIDSuffix.ToString() : BotName));
						PC->CachedPatrollingComponent->SetPatrolPath(PatrolPath);
						bSetupPatrollingComp = true;
						break;
					}
				}
			}
		}

		NpcAI::NpcBot* Bot = new NpcAI::NpcBot(PC, Pawn, PlayerState);
		NpcAI::NpcBots.push_back(Bot);
		Bot->BT_NPC = NpcAI::ConstructBehaviorTree();
		AmountTimesCalled++;
	}

	void (*InventoryBaseOnSpawnedOG)(UFortAthenaAISpawnerDataComponent_InventoryBase* a1, APawn* a2);
	void InventoryBaseOnSpawned(UFortAthenaAISpawnerDataComponent_InventoryBase* a1, APawn* Pawn)
	{
		if (!Pawn || !Pawn->Controller)
			return;
		auto PC = (AFortAthenaAIBotController*)Pawn->Controller;

		if (!PC->Inventory)
			PC->Inventory = SpawnActor<AFortInventory>({}, {}, PC);

		InventoryBaseOnSpawnedOG(a1, Pawn);
	}

	void (*OnPossessedPawnDiedOG)(AFortAthenaAIBotController* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum);
	void OnPossessedPawnDied(AFortAthenaAIBotController* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum)
	{
		if (!PC) {
			return;
		}

		UClass* BotSpawnerData = nullptr;
		for (BotSpawnData& SpawnedBot : SpawnedBots) {
			if (SpawnedBot.Controller == PC) {
				if (SpawnedBot.BotSpawnerData) {
					BotSpawnerData = SpawnedBot.BotSpawnerData;
				}
			}
		}
		if (BotSpawnerData) {
			UFortAthenaAIBotSpawnerData* SpawnerData = Cast<UFortAthenaAIBotSpawnerData>(BotSpawnerData->DefaultObject);
			if (!SpawnerData) {
				return;
			}

			UFortAthenaAISpawnerDataComponent_AIBotInventory* InventoryComponent = (UFortAthenaAISpawnerDataComponent_AIBotInventory*)SpawnerData->GetInventoryComponent();
			if (InventoryComponent->ShouldDropCurrencyOnDeath.Value) {
				static auto Bars = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/Athena_WadsItemData.Athena_WadsItemData");

				SpawnPickup(Bars, UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(10, 150), 0, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::BotElimination);
			}

			if (PC->Inventory) {
				for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass())) {
						continue;
					}
					if (!((UFortWorldItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped) {
						continue;
					}
					auto Def = PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition;
					SpawnPickup(Def, 0, 0, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::BotElimination);
					UFortAmmoItemDefinition* AmmoDef = (UFortAmmoItemDefinition*)((UFortWeaponRangedItemDefinition*)Def)->GetAmmoWorldItemDefinition_BP();
					if (AmmoDef) {
						SpawnPickup(AmmoDef, AmmoDef->DropCount, 0, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::BotElimination);
					}
				}
			}
		}

		return;
		//return OnPossessedPawnDiedOG(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum);
	}

	wchar_t* (*OnPerceptionSensedOG)(ABP_PhoebePlayerController_C* PC, AActor* SourceActor, FAIStimulus& Stimulus);
	wchar_t* OnPerceptionSensed(AFortAthenaAIBotController* PC, AActor* SourceActor, FAIStimulus& Stimulus) {
		if (!PC || !SourceActor) {
			return nullptr;
		}

		for (FortAthenaAIBotController::BotSpawnData& SpawnedBot : FortAthenaAIBotController::SpawnedBots) {
			if (!SpawnedBot.Controller || !SpawnedBot.Pawn || !SpawnedBot.PlayerState)
				continue;

			if (SpawnedBot.Controller == PC) {
				if (SpawnedBot.Controller->CurrentAlertLevel == EAlertLevel::Threatened) {
					SpawnedBot.Controller->Blackboard->SetValueAsEnum(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_RangeAttack_ExecutionStatus"), (int)EExecutionStatus::ExecutionAllowed);
					SpawnedBot.Controller->Blackboard->SetValueAsBool(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_ManageWeapon_Fire"), true);
				}
			}
		}
	}

	float (*GetTrackingModifierInternalOG)(UFortAthenaAIBotAimingDigestedSkillSet* This, int Curve, double SignNegationProbability);
	float GetTrackingModifierInternal(UFortAthenaAIBotAimingDigestedSkillSet* This, int Curve, double SignNegationProbability) {
		return 0.f;
		float TrackingModifier = GetTrackingModifierInternal(This, Curve, SignNegationProbability);
		Log("TrackingModifier: " + std::to_string(TrackingModifier));
		
		return TrackingModifier;
	}

	FDigestedWeaponAccuracy* (*GetWeaponAccuracyOG)(UFortAthenaAIBotAimingDigestedSkillSet* This, AFortWeapon* Weapon);
	FDigestedWeaponAccuracy* GetWeaponAccuracy(UFortAthenaAIBotAimingDigestedSkillSet* This, AFortWeapon* Weapon) {
		if (!This || !Weapon || !Weapon->WeaponData) return nullptr;

		/*auto TagName = Weapon->WeaponData->AnalyticTags.GameplayTags[0].TagName;
		Log(TagName.ToString());

		Log("WeaponAccuraciesNum: " + std::to_string(This->WeaponAccuracies.Num()));

		FDigestedWeaponAccuracy* WeaponAccuracy = nullptr;
		for (FDigestedWeaponAccuracyCategory& WeaponAccuracyCatagory : This->WeaponAccuracies) {
			for (FGameplayTag Tag : WeaponAccuracyCatagory.Tags.GameplayTags) {
				Log("WeaponGameplayTag: " + Tag.TagName.ToString());
			}

			for (FGameplayTag Tag : WeaponAccuracyCatagory.Tags.ParentTags) {
				Log("WeaponParentTag: " + Tag.TagName.ToString());
			}

			WeaponAccuracy = &WeaponAccuracyCatagory.WeaponAccuracy;
		}*/

		FDigestedWeaponAccuracy* WeaponAccuracy = GetWeaponAccuracyOG(This, Weapon);

		return WeaponAccuracy;
	}

	void HookAll() {
		MH_CreateHook((LPVOID)(ImageBase + 0x2047EA4), CreateAndConfigureNavigationSystem, (LPVOID*)&CreateAndConfigureNavigationSystemOG);

		HookVTable(UAthenaNavSystem::GetDefaultObj(), 0x55, InitializeForWorld, (LPVOID*)&InitializeForWorldOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x4509720), OnPawnAISpawned, (LPVOID*)&OnPawnAISpawnedOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x46C5688), InventoryBaseOnSpawned, (LPVOID*)&InventoryBaseOnSpawnedOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x450A108), OnPossessedPawnDied, (LPVOID*)&OnPossessedPawnDiedOG);

		//MH_CreateHook((LPVOID)(ImageBase + 0x467FE94), GetTrackingModifierInternal, (LPVOID*)&GetTrackingModifierInternalOG);

		//MH_CreateHook((LPVOID)(ImageBase + 0x4680088), GetWeaponAccuracy, (LPVOID*)&GetWeaponAccuracyOG);

		UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogAthenaAIServiceBots VeryVerbose", nullptr);
		UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogAthenaBots VeryVerbose", nullptr);
		Log("Hooked FortAIBotControllerAthena!");
	}
}