// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyAbilityCoolDownEffect.h"

USpyAbilityCoolDownEffect::USpyAbilityCoolDownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(1.0f);
	FInheritedTagContainer InheritableTags = FInheritedTagContainer();
	InheritableTags.Added = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Activation.Fail.OnCooldown"));
	InheritableTags.CombinedTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Activation.Fail.OnCooldown"));
	InheritableOwnedTagsContainer = InheritableTags;
}
