#pragma once
#include "framework.h"
#include "BehaviorTree_System.h"

namespace NpcAI {
	struct BT_NPC_Context : BTContext
	{
		class NpcBot* bot;
	};

	std::vector<class NpcBot*> NpcBots{};
	class NpcBot
	{
	public:
		// The behaviortree for the new ai system
		BehaviorTree* BT_NPC = nullptr;

		// The context that should be sent to the behaviortree
		BT_NPC_Context Context = {};

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
		NpcBot(AFortAthenaAIBotController* PC, AFortPlayerPawnAthena* Pawn, AFortPlayerStateAthena* PlayerState)
		{
			this->PC = PC;
			this->Pawn = Pawn;
			this->PlayerState = PlayerState;

			Context.Controller = PC;
			Context.Pawn = Pawn;
			Context.PlayerState = PlayerState;
			Context.bot = this;

			NpcBots.push_back(this);
		}
	};
}