#pragma once
#include "framework.h"
#include "BehaviorTree_System.h"

namespace PlayerBots {
	struct BT_Phoebe_Context : BTContext
	{
		class PhoebeBot* bot;
	};

	std::vector<class PhoebeBot*> PhoebeBots{};
	class PhoebeBot
	{
	public:
		// The behaviortree for the new ai system
		BehaviorTree* BT_Phoebe = nullptr;

		// The context that should be sent to the behaviortree
		BT_Phoebe_Context Context = {};

		// The playercontroller of the bot
		AFortAthenaAIBotController* PC;

		// The Pawn of the bot
		AFortPlayerPawnAthena* Pawn;

		// The PlayerState of the bot
		AFortPlayerStateAthena* PlayerState;

		// Are we ticking the bot?
		bool bTickEnabled = true;

		// So we can track the current tick that the bot is doing
		uint64_t tick_counter = 0;

	public:
		PhoebeBot(AFortAthenaAIBotController* PC, AFortPlayerPawnAthena* Pawn, AFortPlayerStateAthena* PlayerState)
		{
			this->PC = PC;
			this->Pawn = Pawn;
			this->PlayerState = PlayerState;

			Context.Controller = PC;
			Context.Pawn = Pawn;
			Context.PlayerState = PlayerState;
			Context.bot = this;

			PhoebeBots.push_back(this);
		}

		bool IsPickaxeEquiped() {
			if (!Pawn || !Pawn->CurrentWeapon)
				return false;

			if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
			{
				return true;
			}
			return false;
		}

		void EquipPickaxe()
		{
			if (!Pawn || !Pawn->CurrentWeapon)
				return;

			if (IsPickaxeEquiped()) {
				return;
			}

			for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
				{
					Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition, PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid, PC->Inventory->Inventory.ReplicatedEntries[i].TrackerGuid, false);
					break;
				}
			}
		}

		void SwitchToWeapon() {
			if (!Pawn || !Pawn->CurrentWeapon || !Pawn->CurrentWeapon->WeaponData || !PC || !PC->Inventory)
				return;

			if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass())) {
				return;
			}

			if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
			{
				for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
					if (Entry.ItemDefinition) {
						if (Entry.ItemDefinition->ItemType == EFortItemType::Weapon) {
							Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, Entry.ItemGuid, Entry.TrackerGuid, false);
							break;
						}
					}
				}
			}
		}
	};
}