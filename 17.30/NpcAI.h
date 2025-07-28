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

	namespace BT_NPC_Tasks {
		class BTTask_Wait : public BTNode {
		public:
			float WaitTime = 0.f;
			float WorldWaitTime = 0.f;
			bool bFinishedWait = false;
		public:
			BTTask_Wait(float InWaitTime) {
				WaitTime = InWaitTime;
				WorldWaitTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) + InWaitTime;
			}

			virtual EBTNodeResult ChildTask(BTContext Context) override {
				if (WaitTime == 0.f || WorldWaitTime == 0.f) {
					return EBTNodeResult::Failed;
				}
				if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) >= WorldWaitTime) {
					// Reset the wait if the wait is completed
					if (bFinishedWait) {
						WorldWaitTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) + WaitTime;
						bFinishedWait = false;
						return EBTNodeResult::InProgress;
					}
					bFinishedWait = true;
					return EBTNodeResult::Succeeded;
				}
				// If it is still waiting then return inprogress
				return EBTNodeResult::InProgress;
			}
		};

		class BTTask_Dive : public BTNode {
		public:
			virtual EBTNodeResult ChildTask(BTContext Context) override {
				if (!Context.Pawn || !Context.Controller) {
					return EBTNodeResult::Failed;
				}

				if (Context.Pawn->IsSkydiving()) {
					FVector LocationToGo = Context.Pawn->K2_GetActorLocation();
					LocationToGo.Z -= 100.f;

					Context.Controller->K2_SetFocalPoint(LocationToGo);
					Context.Controller->MoveToLocation(LocationToGo, 20.f, true, false, false, true, nullptr, true);
				}
				else {
					Context.Pawn->BeginSkydiving(false);
				}

				return EBTNodeResult::Succeeded;
			}
		};

		class BTTask_RunSelector : public BTNode {
		public:
			BTComposite_Selector* SelectorToRun = nullptr;
		public:
			virtual EBTNodeResult ChildTask(BTContext Context) override {
				if (SelectorToRun) {
					return SelectorToRun->Tick(Context);
				}
				return EBTNodeResult::Failed;
			}
		};

		class BTTask_Glide : public BTNode {
		public:
			virtual EBTNodeResult ChildTask(BTContext Context) override {
				if (!Context.Pawn || !Context.Controller) {
					return EBTNodeResult::Failed;
				}

				// I dont think we need any custom logic right now for gliding
				return EBTNodeResult::Succeeded;
			}
		};

		class BTTask_SteerMovement : public BTNode {
		public:
			virtual EBTNodeResult ChildTask(BTContext Context) override {
				if (!Context.Pawn || !Context.Controller) {
					return EBTNodeResult::Failed;
				}
				//Log("SteerMovement Task Called");

				// TODO: Fix the bot moving, The bot currently just stands still and does not move
				FVector TargetLocation = Context.Pawn->K2_GetActorLocation();
				TargetLocation.X += UKismetMathLibrary::GetDefaultObj()->RandomFloatInRange(-500.f, 500.f);
				TargetLocation.Y += UKismetMathLibrary::GetDefaultObj()->RandomFloatInRange(-500.f, 500.f);
				EPathFollowingRequestResult Result = Context.Controller->MoveToLocation(TargetLocation, 20.f, true, false, false, true, nullptr, true);
				if (Result == EPathFollowingRequestResult::Failed) {
					Log("SteerMovement: Task failed to execute MoveToLocation!");
					return EBTNodeResult::Failed;
				}
				else if (Result == EPathFollowingRequestResult::RequestSuccessful) {
					return EBTNodeResult::InProgress;
				}
				else if (Result == EPathFollowingRequestResult::AlreadyAtGoal) {
					return EBTNodeResult::Succeeded;
				}

				return EBTNodeResult::Succeeded;
			}
		};
	}

	namespace BT_NPC_Decorators {
		class BTDecorator_BlackBoard_Enum : public BTDecorator {
		public:
			FName SelectedKeyName;
			int IntValue = 0;
			EBlackboardCompareOp Operator = EBlackboardCompareOp::Equal;
		public:
			virtual bool Evaluate(BTContext Context) override {
				if (!Context.Controller || !Context.Controller->Blackboard) {
					return false;
				}

				const int32 BBValue = Context.Controller->Blackboard->GetValueAsEnum(SelectedKeyName);

				switch (Operator)
				{
				case EBlackboardCompareOp::Equal:               return BBValue == IntValue;
				case EBlackboardCompareOp::NotEqual:            return BBValue != IntValue;
				case EBlackboardCompareOp::LessThan:            return BBValue < IntValue;
				case EBlackboardCompareOp::LessThanOrEqual:     return BBValue <= IntValue;
				case EBlackboardCompareOp::GreaterThan:         return BBValue > IntValue;
				case EBlackboardCompareOp::GreaterThanOrEqual:  return BBValue >= IntValue;
				default:                                        return false;
				}
			}
		};

		class BTDecorator_BlackBoard_IsSet : public BTDecorator {
		public:
			FName SelectedKeyName;
		public:
			virtual bool Evaluate(BTContext Context) override {
				if (!Context.Controller || !Context.Controller->Blackboard) {
					return false;
				}

				const bool BBValue = Context.Controller->Blackboard->GetValueAsBool(SelectedKeyName);
				if (BBValue) {
					return true;
				}
			}
		};
	}

	namespace BT_NPC_Services {
		class BTService_AIEvaluator_1 : public BTService {
		public:
			BTService_AIEvaluator_1() {
				Name = "FortAthenaBTService_AIEvaluator_1";
				NodeName = "Evaluating...Free Fall";
			}

			virtual void TickService(BTContext Context) override {
				if (!Context.Pawn || !Context.Controller) return;

				if (Context.Pawn->IsSkydiving())
				{
					Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Dive_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionAllowed);
				}
				else
				{
					if (Context.Pawn->IsParachuteOpen()) {
						Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Glide_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionAllowed);
					}
					else {
						Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Dive_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
					}
					Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Glide_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
					Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Dive_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
				}
			}
		};

		class BTService_AIEvaluator_39 : public BTService {
		public:
			BTService_AIEvaluator_39() {
				Name = "FortAthenaBTService_AIEvaluator_39";
				NodeName = "Evaluating...Character Launched";
			}

			virtual void TickService(BTContext Context) override {
				if (!Context.Pawn || !Context.Controller) return;

				if (Context.Pawn->GetMovementComponent()) {
					if (Context.Pawn->GetMovementComponent()->IsFalling()) {
						Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_CharacterLaunched_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionAllowed);
					}
					else {
						Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_CharacterLaunched_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
					}
				}
			}
		};

		class BTService_AIEvaluator_4 : public BTService {
		public:
			BTService_AIEvaluator_4() {
				Name = "FortAthenaBTService_AIEvaluator_4";
				NodeName = "Focus Scan Around Only";

				Interval = 0.3f;
			}

			virtual void TickService(BTContext Context) override {
				if (!Context.Pawn || !Context.Controller) return;
				auto Math = UKismetMathLibrary::GetDefaultObj();

				auto BotPos = Context.Pawn->K2_GetActorLocation();
				FVector FocusLocation = Context.Pawn->K2_GetActorLocation();
				FocusLocation.X += Math->RandomFloatInRange(-100, 100);
				FocusLocation.Y += Math->RandomFloatInRange(-100, 100);

				FRotator Rot = Math->FindLookAtRotation(BotPos, FocusLocation);

				Context.Controller->SetControlRotation(Rot);
				Context.Controller->K2_SetActorRotation(Rot, true);

				Context.Controller->K2_SetFocalPoint(FocusLocation);
			}
		};
	}

	BehaviorTree* ConstructBehaviorTree() {
		auto* Tree = new BehaviorTree();

		auto* RootSelector = new BTComposite_Selector();
		RootSelector->NodeName = "Alive";

		{
			auto* Selector = new BTComposite_Selector();
			Selector->NodeName = "On Ground";

			{
				auto* Task = new BT_NPC_Tasks::BTTask_Wait(0.5f);
				auto* Decorator = new BT_NPC_Decorators::BTDecorator_BlackBoard_Enum();
				Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Global_GamePhaseStep");
				Decorator->IntValue = (int)EAthenaGamePhaseStep::EndGame;
				Decorator->Operator = EBlackboardCompareOp::GreaterThanOrEqual;
				Task->AddDecorator(Decorator);
				Selector->AddChild(Task);
			}

			{
				auto* Task = new BT_NPC_Tasks::BTTask_Wait(1.f);
				auto* Decorator = new BT_NPC_Decorators::BTDecorator_BlackBoard_IsSet();
				Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Global_IsMovementBlocked");
				Task->AddDecorator(Decorator);
				Selector->AddChild(Task);
			}

			{
				auto* Task = new BT_NPC_Tasks::BTTask_SteerMovement();
				auto* Service = new BT_NPC_Services::BTService_AIEvaluator_4();
				Task->AddService(Service);
				auto* Decorator = new BT_NPC_Decorators::BTDecorator_BlackBoard_Enum();
				Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_CharacterLaunched_ExecutionStatus");
				Decorator->IntValue = (int)EExecutionStatus::ExecutionPending;
				Decorator->Operator = EBlackboardCompareOp::GreaterThanOrEqual;
				Task->AddDecorator(Decorator);
				Selector->AddChild(Task);
			}

			{
				auto* Service = new BT_NPC_Services::BTService_AIEvaluator_39();
				Selector->AddService(Service);
			}

			Tree->AllNodes.push_back(Selector);
		}

		{
			auto* Service = new BT_NPC_Services::BTService_AIEvaluator_1();
			RootSelector->AddService(Service);
		}

		{
			auto* Task = new BT_NPC_Tasks::BTTask_Dive();
			auto* Decorator = new BT_NPC_Decorators::BTDecorator_BlackBoard_Enum();
			Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Dive_ExecutionStatus");
			Decorator->IntValue = 4;
			Task->AddDecorator(Decorator);
			RootSelector->AddChild(Task);
		}

		{
			auto* Task = new BT_NPC_Tasks::BTTask_Glide();
			auto* Decorator = new BT_NPC_Decorators::BTDecorator_BlackBoard_Enum();
			Decorator->SelectedKeyName = ConvFName(L"AIEvaluator_Glide_ExecutionStatus");
			Decorator->IntValue = 4;
			Task->AddDecorator(Decorator);
			RootSelector->AddChild(Task);
		}

		{
			auto* Task = new BT_NPC_Tasks::BTTask_RunSelector();
			Task->SelectorToRun = Tree->FindSelectorByName("On Ground");
			if (Task->SelectorToRun) {
				RootSelector->AddChild(Task);
			}
		}

		Tree->RootNode = RootSelector;
		Tree->AllNodes.push_back(RootSelector);

		return Tree;
	}

	void TickBots() {
		for (auto bot : NpcBots)
		{
			if (bot->BT_NPC) {
				bot->BT_NPC->Tick(bot->Context);
			}
		}
	}
}