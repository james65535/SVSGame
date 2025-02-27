// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SpyVsSpy/SpyVsSpy.h"
#include "SpyGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API USpyGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:

	USpyGameplayAbility();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SVS|Abilities")
	ESpyAbilityInputID AbilityInputID { ESpyAbilityInputID::None };

	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities")
	bool AreTasksStillActive();

	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities")
	void PrintActiveTaskNames();
};
