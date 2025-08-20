#pragma once
#include "framework.h"
#include "BehaviorTreeSystem.h"

class BTDecorator_CheckEnum : public BTDecorator {
public:
    FName SelectedKeyName;
    int IntValue = 0;
    EBlackboardCompareOp Operator = EBlackboardCompareOp::Equal;
public:
    bool Evaluate(BTContext Context) override {
        if (!Context.Controller || !Context.Controller->Blackboard) {
            return false;
        }

        int32 BBValue = Context.Controller->Blackboard->GetValueAsEnum(SelectedKeyName);

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

class BTDecorator_IsSet : public BTDecorator {
public:
    FName SelectedKeyName;
public:
    bool Evaluate(BTContext Context) override {
        if (!Context.Controller || !Context.Controller->Blackboard) {
            return false;
        }

        const bool BBValue = Context.Controller->Blackboard->GetValueAsBool(SelectedKeyName);
        if (BBValue == true) {
            return true;
        }
    }
};