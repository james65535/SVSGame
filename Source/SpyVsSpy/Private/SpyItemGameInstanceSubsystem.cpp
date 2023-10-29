// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyItemGameInstanceSubsystem.h"

USpyItemGameInstanceSubsystem::USpyItemGameInstanceSubsystem()
{
	if (IsValid(GetGameInstance()))
	{ UE_LOG(LogTemp, Warning, TEXT("GameInstance: %s Subsystem Hello World!"),
			*GetGameInstance()->GetName()); }
}
