#pragma once
#include "framework.h"
#include "BehaviorTreeSystem.h"

class BTEvaluator_CharacterLaunched : public BTService {
public:
    BTEvaluator_CharacterLaunched() {
        NodeName = "Evaluating...Character Launched";

        Interval = 1.f;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) return;

        UPawnMovementComponent* MovementComp = Context.Pawn->GetMovementComponent();
        if (MovementComp) {
            if (MovementComp->IsFalling()) {
                Context.Controller->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_CharacterLaunched_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionAllowed);
            }
            else {
                Context.Controller->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_CharacterLaunched_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
            }
        }
    }
};

class BTEvaluator_FreeFall : public BTService {
public:
	BTEvaluator_FreeFall() {
		NodeName = "Evaluating...Free Fall";

        Interval = 1.f;
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
				Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Glide_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
			}
			Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Dive_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
		}
	}
};

class BTEvaluator_Escape_EvasiveManeuvers : public BTService {
public:
    float RandDirOffset = 500.f;
    float DirectionChangeInterval = 5.f;
    float NextDirectionChangeTime = 0.f;
public:
    BTEvaluator_Escape_EvasiveManeuvers(float InDirectionChangeInterval = 5.f, float InOffset = 500.f) {
        NodeName = "Evaluating...Escape Evasive Maneuvers";

        Interval = 1.f;
        DirectionChangeInterval = InDirectionChangeInterval;
        RandDirOffset = InOffset;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) return;

        if (Context.Controller->CurrentAlertLevel != EAlertLevel::Threatened)
        {
            Context.Controller->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_AvoidThreat_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
            return;
        }

        float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        if (CurrentTime >= NextDirectionChangeTime)
        {
            FVector AvoidDirection = Context.Pawn->K2_GetActorLocation();
            AvoidDirection.X += UKismetMathLibrary::RandomFloatInRange((RandDirOffset * -1.f), RandDirOffset);
            AvoidDirection.Y += UKismetMathLibrary::RandomFloatInRange((RandDirOffset * -1.f), RandDirOffset);

            Context.Controller->Blackboard->SetValueAsVector(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_AvoidThreat_Destination"), AvoidDirection);
            NextDirectionChangeTime = CurrentTime + DirectionChangeInterval;
        }

        if (Context.Controller->GetMoveStatus() != EPathFollowingStatus::Moving) {
            Context.Controller->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_AvoidThreat_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionAllowed);
        }
        else {
            Context.Controller->Blackboard->SetValueAsEnum(UKismetStringLibrary::Conv_StringToName(L"AIEvaluator_AvoidThreat_ExecutionStatus"), (uint8)EExecutionStatus::ExecutionDenied);
        }
    }
};

class BTEvaluator_MovementBlocked : public BTService {
public:
    BTEvaluator_MovementBlocked() {
        NodeName = "Evaluating...MovementBlocked";
    }

    virtual void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) return;

        Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_Global_IsMovementBlocked"), false);
    }
};