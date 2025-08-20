#pragma once
#include "framework.h"

enum class EBlackboardCompareOp
{
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual
};

struct BTContext
{
    AFortAthenaAIBotController* Controller;
    AFortPlayerPawnAthena* Pawn;
    AFortPlayerStateAthena* PlayerState;
};

class BTDecorator {
public:
    // All of these strings dont need to have a value its just good for runtime data
    std::string Name;
    std::string CachedDescription;
    std::string NodeName;

public:
    virtual bool Evaluate(BTContext Context) = 0;
};

class BTService {
public:
    std::string Name;
    std::string NodeName;

    float Interval = 0.f;
    float NextTickTime = 0.f;
public:
    virtual void TickService(BTContext Context) = 0;
};

class BTNode
{
private:
    std::vector<BTDecorator*> Decorators;
    std::vector<BTService*> Services;
public:
    std::string Name;
    std::string NodeName;
public:
    virtual EBTNodeResult ChildTask(BTContext Context) = 0;
public:
    void AddDecorator(BTDecorator* Decorator) {
        Decorators.push_back(Decorator);
    }

    void AddService(BTService* Service) {
        Services.push_back(Service);
    }

    EBTNodeResult Tick(BTContext& Context) {
        float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

        // If a decorator fails then we shouldnt execute the task
        for (BTDecorator* Decorator : Decorators) {
            if (!Decorator->Evaluate(Context)) {
                return EBTNodeResult::Failed;
            }
        }

        for (BTService* Service : Services) {
            if (Service->Interval == 0.f || CurrentTime >= Service->NextTickTime) {
                Service->TickService(Context);
                Service->NextTickTime = CurrentTime + Service->Interval;
            }
        }

        // Run the task once all of the decorators pass
        return ChildTask(Context);
    }
};

class BTComposite_Selector
{
private:
    std::vector<BTNode*> Children;
    std::vector<BTDecorator*> Decorators;
    std::vector<BTService*> Services;

public:
    std::string Name;
    std::string NodeName;
public:
    void AddChild(BTNode* Node) {
        Children.push_back(Node);
    }

    void AddDecorator(BTDecorator* Decorator) {
        Decorators.push_back(Decorator);
    }

    void AddService(BTService* Service) {
        Services.push_back(Service);
    }

    virtual EBTNodeResult Tick(BTContext Context) {
        float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

        // If a global selector decorator fails we shouldnt execute anything inside of the selector (shouldnt be on the root)
        for (BTDecorator* Decorator : Decorators) {
            if (!Decorator->Evaluate(Context)) {
                return EBTNodeResult::Failed;
            }
        }

        for (BTService* Service : Services) {
            if (Service->Interval == 0.f || CurrentTime >= Service->NextTickTime) {
                Service->TickService(Context);
                Service->NextTickTime = CurrentTime + Service->Interval;
            }
        }

        // Run all of the selectors children then if all fail then return faliure
        for (BTNode* Child : Children)
        {
            EBTNodeResult Result = Child->Tick(Context);
            if (Result == EBTNodeResult::Succeeded || Result == EBTNodeResult::InProgress)
                return Result;
        }
        return EBTNodeResult::Failed;
    }
};

class BehaviorTree
{
public:
    std::string Name;

    std::vector<BTComposite_Selector*> AllNodes;
    BTComposite_Selector* RootNode = nullptr;
    UBlackboardData* BlackboardAsset = nullptr;

    BTComposite_Selector* FindSelectorByName(std::string Name) {
        for (BTComposite_Selector* Selector : AllNodes) {
            if (Selector->Name == Name || Selector->NodeName == Name) {
                return Selector;
            }
        }

        Log("Selector with name " + Name + " not found!");
        return nullptr;
    }

    void Tick(BTContext Context) {
        if (RootNode)
            RootNode->Tick(Context);
    }

    ~BehaviorTree() {
        for (auto* Node : AllNodes)
            delete Node;
    }
};