#pragma once
#include "framework.h"
#include "BehaviorTreeSystem.h"

class BTService_HandleFocusing_ScanAroundOnly : public BTService {
public:
    float Offset = 100.f;
public:
    BTService_HandleFocusing_ScanAroundOnly(float InInterval = 0.3f, float InOffset = 100.f) {
        NodeName = "Focus Scan Around Only";

        Interval = InInterval;
        Offset = InOffset;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) return;
        auto Math = UKismetMathLibrary::GetDefaultObj();

        auto BotPos = Context.Pawn->K2_GetActorLocation();
        FVector FocusLocation = Context.Pawn->K2_GetActorLocation();
        FocusLocation.X += UKismetMathLibrary::RandomFloatInRange((Offset * -1.f), Offset);
        FocusLocation.Y += UKismetMathLibrary::RandomFloatInRange((Offset * -1.f), Offset);
        FocusLocation.Z += UKismetMathLibrary::RandomFloatInRange(((Offset / 2) * -1.f), (Offset / 2));

        FRotator Rot = Math->FindLookAtRotation(BotPos, FocusLocation);

        Context.Controller->SetControlRotation(Rot);
        Context.Controller->K2_SetActorRotation(Rot, true);

        Context.Controller->K2_SetFocalPoint(FocusLocation);
    }
};

class BTService_ThankBusDriver : public BTService {
public:
    BTService_ThankBusDriver(float InInterval = 0.2f) {
        NodeName = "Random Thank Bus Driver";

        Interval = InInterval;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller || !Context.PlayerState) return;
        
        if (!Context.PlayerState->bInAircraft) return;
        if (Context.PlayerState->bThankedBusDriver) return;

        if (UKismetMathLibrary::RandomBoolWithWeight(0.005f)) {
            Context.Controller->ThankBusDriver();
        }
    }
};

class BTService_JumpOffBus : public BTService {
public:
    float Weight = 0.005f;
public:
    BTService_JumpOffBus(float InWeight = 0.005f) {
        NodeName = "Executing... Jump Off The Bus";

        Interval = UKismetMathLibrary::RandomFloatInRange(0.05f, 0.2f);
        Weight = InWeight;
    }

    void TickService(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller || !Context.PlayerState) return;

        if (!Context.PlayerState->bInAircraft) return;

        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        if (GameState->bAircraftIsLocked) return;

        if (UKismetMathLibrary::RandomBoolWithWeight(Weight)) {
            FVector Destination = Context.Controller->Blackboard->GetValueAsVector(ConvFName(L"AIEvaluator_JumpOffBus_Destination"));

            Context.Controller->Blackboard->SetValueAsVector(ConvFName(L"AIEvaluator_Dive_Destination"), Destination);
            Context.Controller->Blackboard->SetValueAsVector(ConvFName(L"AIEvaluator_Glide_Destination"), Destination);

            Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Dive_ExecutionStatus"), (int)EExecutionStatus::ExecutionAllowed);
            Context.Controller->Blackboard->SetValueAsEnum(ConvFName(L"AIEvaluator_Glide_ExecutionStatus"), (int)EExecutionStatus::ExecutionDenied);

            Context.Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
            Context.Pawn->BeginSkydiving(true);

            Context.PlayerState->bInAircraft = false;
            Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_Global_IsInBus"), false);

            Context.Controller->Blackboard->SetValueAsBool(ConvFName(L"AIEvaluator_Global_HasEverJumpedFromBusKey"), true);
        }
    }
};