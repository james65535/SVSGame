// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyDamageEffect.h"

void USpyDamageEffect::PostLoad()
{
	Super::PostLoad();

	UE_LOG(SVSLog, Log, TEXT("SPY GE PostLoad")); 
}
