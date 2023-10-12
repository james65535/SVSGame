// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SpyAbilitySystemComponent.generated.h"

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
	bool bCharacterAbilitiesGiven ;
	
};
