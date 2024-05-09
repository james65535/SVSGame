// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/DamageEffectExecCalculation.h"

#include "SVSLogger.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyAttributeSet.h"

struct FSpyDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(DefensePower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Damage);

	FSpyDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USpyAttributeSet, DefensePower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USpyAttributeSet, AttackPower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USpyAttributeSet, Damage, Source, true);
	}
};

static const FSpyDamageStatics& DamageStatics()
{
	static FSpyDamageStatics DmgStatics;
	return DmgStatics;
}

UDamageEffectExecCalculation::UDamageEffectExecCalculation()
{
	RelevantAttributesToCapture.Add(DamageStatics().DefensePowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamageDef);
}

void UDamageEffectExecCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	USpyAbilitySystemComponent* SourceAbilitySystemComponent = Cast<USpyAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());
	USpyAbilitySystemComponent* TargetAbilitySystemComponent = Cast<USpyAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	
	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
	
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	float DefensePower = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().DefensePowerDef,
		EvaluationParameters,
		DefensePower);

	/** Avoid Divide by Zero In Damage Calculation */
	if (DefensePower == 0.0f)
	{ DefensePower = 1.0f; }

	float AttackPower = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().AttackPowerDef,
		EvaluationParameters,
		AttackPower);

	float Damage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().DamageDef,
		EvaluationParameters,
		Damage);
	
	const float DamageDone = Damage * AttackPower / DefensePower;
	
	if (DamageDone > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				DamageStatics().DamageProperty,
				EGameplayModOp::Additive,
				DamageDone));
	}

	/** Broadcast damages to Target ASC */
	if (IsValid(TargetAbilitySystemComponent) && IsValid(SourceAbilitySystemComponent))
	{ TargetAbilitySystemComponent->ReceiveDamage(SourceAbilitySystemComponent, DamageDone); }
}
