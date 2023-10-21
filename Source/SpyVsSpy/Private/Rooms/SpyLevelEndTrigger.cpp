// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SpyLevelEndTrigger.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "Players/SpyCharacter.h"


void ASpyLevelEndTrigger::BeginPlay()
{
	Super::BeginPlay();

	SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();
	if(IsValid(SpyGameState))
	{ OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin); }
}

void ASpyLevelEndTrigger::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (HasAuthority())
	{
		if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(OtherActor))
		{ SpyGameState->OnPlayerReachedEnd(SpyCharacter); }
	}
}
