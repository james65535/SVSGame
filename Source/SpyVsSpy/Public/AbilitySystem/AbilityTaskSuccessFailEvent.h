// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTaskSuccessFailEvent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitSuccessFailEventDelegate, FGameplayEventData, Payload);

/**
 * Gameplay Ability Task Which Allows for A Success or Failure Response Path
 */
UCLASS()
class SPYVSSPY_API UAbilityTaskSuccessFailEvent : public UAbilityTask
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FWaitSuccessFailEventDelegate	SuccessEventReceived;
	UPROPERTY(BlueprintAssignable)
	FWaitSuccessFailEventDelegate	FailEventReceived;
	/**
	 * Wait until the specified gameplay tag event is triggered.
	 * Allows for Success or Failure
	 * By default this will look at the owner of this ability. OptionalExternalTarget can be set to make this look at another actor's tags for changes
	 * It will keep listening as long as OnlyTriggerOnce = false
	 * If OnlyMatchExact = false it will trigger for nested tags
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTaskSuccessFailEvent* WaitSuccessFailEvent(UGameplayAbility* OwningAbility, const FGameplayTag SuccessTag, const FGameplayTag FailTag, AActor* OptionalExternalTarget, const bool OnlyTriggerOnce,  const bool OnlyMatchExact);
	FDelegateHandle SuccessHandle;
	FDelegateHandle FailHandle;
	
protected:
	void SetExternalTarget(const AActor* Actor);

	UAbilitySystemComponent* GetTargetAbilitySystemComponent() const;

	virtual void Activate() override;

	virtual void SuccessEventCallback(const FGameplayEventData* Payload);
	virtual void SuccessEventContainerCallback(FGameplayTag MatchingTag, const FGameplayEventData* Payload);
	virtual void FailEventCallback(const FGameplayEventData* Payload);
	virtual void FailEventContainerCallback(FGameplayTag MatchingTag, const FGameplayEventData* Payload);
	
	virtual void OnDestroy(bool AbilityEnding) override;

	FGameplayTag SuccessTag;
	FGameplayTag FailTag;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> OptionalExternalTarget;

	bool UseExternalTarget;	
	bool OnlyTriggerOnce;
	bool OnlyMatchExact;


};
