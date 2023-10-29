// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyAttributeSet.h"

#include "AttributeSet.h"
#include "GameplayEffectExtension.h"
#include "SVSLogger.h"
#include "net/UnrealNetwork.h"

USpyAttributeSet::USpyAttributeSet()
{
}

void USpyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.Condition = COND_None;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;

	//DOREPLIFETIME_WITH_PARAMS_FAST()
	DOREPLIFETIME_WITH_PARAMS(USpyAttributeSet, Health, SharedParams)
	DOREPLIFETIME_WITH_PARAMS(USpyAttributeSet, MaxHealth, SharedParams);
	DOREPLIFETIME_WITH_PARAMS(USpyAttributeSet, Damage, SharedParams);
	DOREPLIFETIME_WITH_PARAMS(USpyAttributeSet, DefensePower, SharedParams);
	DOREPLIFETIME_WITH_PARAMS(USpyAttributeSet, AttackPower, SharedParams);
}

void USpyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);


	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(
			Health,
			MaxHealth,
			NewValue,
			GetHealthAttribute());
	}
}
// TODO where is this? Replaced by damage calc? Does damage calc modify health like this?
void USpyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamageDone = GetDamage();
		SetDamage(0.f);
		const float NewHealth = GetHealth() - LocalDamageDone;

		SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Handle other health changes.
		// Health loss should go through Damage.
		const float LocalHealth = GetHealth();
		
		SetHealth(FMath::Clamp(LocalHealth, 0.0f, GetMaxHealth()));
	}
}

void USpyAttributeSet::AdjustAttributeForMaxChange(
	const FGameplayAttributeData& AffectedAttribute,
	const FGameplayAttributeData& MaxAttribute, float NewMaxValue,
	const FGameplayAttribute& AffectedAttributeProperty) const
{
	UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent();
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && AbilityComp)
	{
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		const float NewDelta = (CurrentMaxValue > 0.0f) ?
				(CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue :
				NewMaxValue;

		AbilityComp->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}

void USpyAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Health OnRep with Value: %f"), OldValue.GetBaseValue());
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyAttributeSet, Health, OldValue);
}

void USpyAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyAttributeSet, MaxHealth, OldValue);
}

void USpyAttributeSet::OnRep_Damage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyAttributeSet, Damage, OldValue);
}

void USpyAttributeSet::OnRep_DefensePower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyAttributeSet, DefensePower, OldValue);
}

void USpyAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyAttributeSet, AttackPower, OldValue);
}
