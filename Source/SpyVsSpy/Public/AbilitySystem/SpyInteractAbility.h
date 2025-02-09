// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpyDamageEffect.h"
#include "AbilitySystem/SpyGameplayAbility.h"
#include "SpyInteractAbility.generated.h"

class UAbilityTaskSuccessFailEvent;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API USpyInteractAbility : public USpyGameplayAbility
{
	GENERATED_BODY()

public:
	
	/** SpyGameplayAbility Overrides */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;


protected:
	
	/** Used to calculate and apply damage. Needs to be set in child blueprint */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	TSoftClassPtr<USpyDamageEffect> SpyTrapDamageEffectClass;

	/** Allows for blueprint behaviour to be defined if an interaction is possible */
	UFUNCTION(BlueprintNativeEvent)
	void InteractionSuccess(FGameplayEventData Payload);
	/** Allows for blueprint behaviour to be defined if an interaction is not possible */
	UFUNCTION(BlueprintNativeEvent)
	void InteractionFail(FGameplayEventData Payload);

	/** Used as a callback for checking if a trap hit tag has been applied to avatar */
	UFUNCTION()
	void OnTrapTriggered(FGameplayEventData Payload);
	/** Used as a callback for checking if a trap no hit tag has been applied to avatar */
	UFUNCTION()
	void OnTrapNotTriggered(FGameplayEventData Payload);
	UPROPERTY()
	UAbilityTaskSuccessFailEvent* CheckTrapTriggeredTask;
	
	bool CanInteract() const;

	/** Triggers the trap if there is one */
	UFUNCTION(BlueprintCallable)
	bool RequestTriggerTrap();
	
};
