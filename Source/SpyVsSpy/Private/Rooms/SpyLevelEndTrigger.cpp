// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SpyLevelEndTrigger.h"

#include "Players/SpyCharacter.h"
#include "Players/SpyPlayerState.h"


void ASpyLevelEndTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
}

void ASpyLevelEndTrigger::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(OtherActor))
	{
		ASpyPlayerState* SpyPlayerState = SpyCharacter->GetSpyPlayerState();
		if (IsValid(SpyPlayerState) && HasAuthority())
		{ SpyPlayerState->OnPlayerReachedEnd(); }
	}
}
