// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerVolume.h"
#include "SpyLevelEndTrigger.generated.h"

class ASpyVsSpyGameState;
class ASpyVsSpyGameMode;
/**
 * 
 */
UCLASS()
class SPYVSSPY_API ASpyLevelEndTrigger : public ATriggerVolume
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

private:

	// Custom Overlap function to override the actor BeginOverLap
	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	UPROPERTY()
	ASpyVsSpyGameMode* GameModeRef;
	UPROPERTY()
	ASpyVsSpyGameState* SpyGameState;
};
