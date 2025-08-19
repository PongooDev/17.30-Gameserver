#pragma once
#include "framework.h"

namespace AbilitySystemComponent {
	void GiveAbility(UFortGameplayAbility* Ability, AFortPlayerState* PS)
	{
		if (!Ability || !PS) {
			return;
		}

		FGameplayAbilitySpec Spec{};
		AbilitySpecConstructor(&Spec, Ability, 1, -1, nullptr);
		GiveAbilityOG(PS->AbilitySystemComponent, &Spec.Handle, Spec);
	}

	void GiveAbilitySet(UFortAbilitySet* AbilitySet, AFortPlayerState* PS)
	{
		if (!AbilitySet || !PS) {
			Log("Cannot Give AbilitySet! NULL!");
			Log("AbilitySet: " + AbilitySet->GetFullName());
			Log("PlayerState: " + PS->GetFullName());
			return;
		}

		for (int i = 0; i < AbilitySet->GameplayAbilities.Num(); i++) {
			GiveAbility((UFortGameplayAbility*)AbilitySet->GameplayAbilities[i].Get()->DefaultObject, PS);
		}

		for (int i = 0; i < AbilitySet->GrantedGameplayEffects.Num(); i++) {
			UClass* GameplayEffect = AbilitySet->GrantedGameplayEffects[i].GameplayEffect.Get();
			float Level = AbilitySet->GrantedGameplayEffects[i].Level;

			if (!GameplayEffect)
				continue;

			PS->AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GameplayEffect, Level, FGameplayEffectContextHandle());
		}
	}

	//https://github.com/EpicGames/UnrealEngine/blob/87f8792983fb4228be213b15b57f675dfe143d16/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp#L584
	FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
	{
		for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->ActivatableAbilities.Items)
		{
			if (Spec.Handle.Handle == Handle.Handle)
				return &Spec;
		}

		return nullptr;
	}

	//https://github.com/EpicGames/UnrealEngine/blob/87f8792983fb4228be213b15b57f675dfe143d16/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp#L1445
	void InternalServerTryActiveAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilitySystemComponent, Handle);
		if (!Spec)
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			return;
		}

		const UGameplayAbility* AbilityToActivate = Spec->Ability;

		UGameplayAbility* InstancedAbility = nullptr;
		Spec->InputPressed = true;

		if (!InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			Spec->InputPressed = false;

			AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
		}
	}

	void HookAll()
	{
		int InternalServerTryActiveAbilityIndex = 0xFE;

		HookVTable(UAbilitySystemComponent::GetDefaultObj(), InternalServerTryActiveAbilityIndex, InternalServerTryActiveAbility, nullptr);
		HookVTable(UFortAbilitySystemComponent::GetDefaultObj(), InternalServerTryActiveAbilityIndex, InternalServerTryActiveAbility, nullptr);
		HookVTable(UFortAbilitySystemComponentAthena::GetDefaultObj(), InternalServerTryActiveAbilityIndex, InternalServerTryActiveAbility, nullptr);
	}
}