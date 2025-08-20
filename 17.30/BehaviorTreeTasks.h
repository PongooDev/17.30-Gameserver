#pragma once
#include "framework.h"
#include "BehaviorTreeSystem.h"

class BTTask_Wait : public BTNode {
public:
    float WaitTime = 0.f;
    float WorldWaitTime = 0.f;
    bool bFinishedWait = false;
public:
    BTTask_Wait(float InWaitTime) {
        WaitTime = InWaitTime;
        WorldWaitTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + InWaitTime;
    }

    EBTNodeResult ChildTask(BTContext Context) override
    {
        if (WaitTime == 0.f || WorldWaitTime == 0.f) {
            return EBTNodeResult::Failed;
        }
        if (UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) >= WorldWaitTime) {
            if (bFinishedWait) {
                WorldWaitTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + WaitTime;
                bFinishedWait = false;
                return EBTNodeResult::InProgress;
            }
            bFinishedWait = true;
            if (!NodeName.empty())
            {
                Log("BTTask_Wait Finished Wait For NodeName: " + NodeName);
            }
            return EBTNodeResult::Succeeded;
        }
        return EBTNodeResult::InProgress;
    }
};

class BTTask_BotMoveTo : public BTNode {
public:
    float AcceptableRadius = 50.f;
    bool bAllowStrafe = true;
    bool bStopOnOverlapNeedsUpdate = false;
    bool bUsePathfinding = false;
    bool bProjectDestinationToNavigation = false;
    bool bAllowPartialPath = true;

    bool bShouldSetFocalPoint = true;

    FName SelectedKeyName;
public:
    EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Controller) {
            return EBTNodeResult::Failed;
        }

        FVector Dest = Context.Controller->Blackboard->GetValueAsVector(SelectedKeyName);
        if (Dest.IsZero()) return EBTNodeResult::Failed;

        if (bShouldSetFocalPoint)
        {
            Context.Controller->K2_SetFocalPoint(Dest);
        }
        EPathFollowingRequestResult RequestResult = Context.Controller->MoveToLocation(Dest, AcceptableRadius, bStopOnOverlapNeedsUpdate, bUsePathfinding, bProjectDestinationToNavigation, bAllowStrafe, nullptr, bAllowPartialPath);
        if (RequestResult == EPathFollowingRequestResult::Failed) {
            return EBTNodeResult::Failed;
        }

        if (RequestResult == EPathFollowingRequestResult::RequestSuccessful) {
            return EBTNodeResult::InProgress;
        }

        return EBTNodeResult::Succeeded;
    }
};

class BTTask_SteerMovement : public BTNode {
public:
    float RandDirOffset = 600.f;
    float DirectionChangeInterval = 1.f;
    float NextDirectionChangeTime = 0.f;
public:
    BTTask_SteerMovement(float Offset = 600.f, float DestChangeInterval = 1.f)
    {
        RandDirOffset = Offset;
        DirectionChangeInterval = DestChangeInterval;
    }

    EBTNodeResult ChildTask(BTContext Context) override {
        if (!Context.Pawn || !Context.Controller) {
            return EBTNodeResult::Failed;
        }
        float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

        if (DirectionChangeInterval == 0.f || CurrentTime >= NextDirectionChangeTime)
        {
            FVector OffsetLoc = Context.Pawn->K2_GetActorLocation();
            OffsetLoc.X += UKismetMathLibrary::RandomFloatInRange((RandDirOffset * -1.f), RandDirOffset);
            OffsetLoc.Y += UKismetMathLibrary::RandomFloatInRange((RandDirOffset * -1.f), RandDirOffset);

            EPathFollowingRequestResult Result = Context.Controller->MoveToLocation(OffsetLoc, (RandDirOffset / 10), false, false, false, true, nullptr, true);
            NextDirectionChangeTime = CurrentTime + DirectionChangeInterval;

            if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
            {
                return EBTNodeResult::Succeeded;
            }
            else if (Result == EPathFollowingRequestResult::RequestSuccessful)
            {
                return EBTNodeResult::InProgress;
            }
        }

        return EBTNodeResult::Failed;
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

class BTTask_ShootTrap : public BTNode {
public:
    FName TargetActorKey;
public:
    virtual EBTNodeResult ChildTask(BTContext Context) override {
        // I will look into this later, since im not sure the best way to do this
        return EBTNodeResult::Failed;
    }
};