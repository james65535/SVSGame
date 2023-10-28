// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SpyAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReceivedDamageDelegate, const USpyAbilitySystemComponent*, SourceAbilitySystemComponent, float, DamageDone);

/**
 * 
 */
UCLASS()
class SPYVSSPY_API USpyAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	USpyAbilitySystemComponent();

	/**
	 * Boolean to gate the setup of abilities as it can either start in
	 * SetupPlayerInputComponent() or OnRep_PlayerState() due race conditions
	 */
	bool bCharacterAbilitiesGiven = false;
	bool bAbilitySystemInitiated = false;

	FReceivedDamageDelegate ReceivedDamageDelegate;
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Ability")
	void ReceiveDamage(const USpyAbilitySystemComponent* SourceAbilitySystemComponent, float DamageDone);

	UFUNCTION(BlueprintCallable, Category = "SVS|Ability")
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	
};
