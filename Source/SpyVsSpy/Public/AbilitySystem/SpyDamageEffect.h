// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SpyDamageEffect.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API USpyDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:

	virtual void PostLoad() override;
	
	
};
