// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SpyGameplayAbility.h"
#include "SpyAttackAbility.generated.h"

class UAbilityTaskSuccessFailEvent;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API USpyAttackAbility : public USpyGameplayAbility
{
	GENERATED_BODY()

public:

	USpyAttackAbility();
	
	/** SpyGameplayAbility Overrides */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	
	/** Starts the attack sequence */
	UFUNCTION(BlueprintCallable)
	bool RequestAttack() const;

	/** Used for getting result of the attack sequence hit/miss */
	UPROPERTY()
	UAbilityTaskSuccessFailEvent* CheckAttackResultTask;
	FGameplayTag AttackHitTag;
	FGameplayTag AttackMissTag;
	
	/** Used as a callback for checking if a trap hit tag has been applied to avatar */
	UFUNCTION()
	void OnAttackHit(FGameplayEventData Payload);
	/** Used as a callback for checking if a trap no hit tag has been applied to avatar */
	UFUNCTION()
	void OnAttackMiss(FGameplayEventData Payload);

};
