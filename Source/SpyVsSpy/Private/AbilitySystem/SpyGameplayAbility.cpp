// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyGameplayAbility.h"

#include "GameplayTask.h"
#include "SVSLogger.h"

USpyGameplayAbility::USpyGameplayAbility()
{
}

bool USpyGameplayAbility::AreTasksStillActive()
{
	return (ActiveTasks.Num() > 0);
}

void USpyGameplayAbility::PrintActiveTaskNames()
{
	for (const UGameplayTask* ActiveTask : ActiveTasks)
	{
		UE_LOG(SVSLogDebug, Log, TEXT("Active Task Instance Name: %s"),
		*ActiveTask->GetName());
	}
}
