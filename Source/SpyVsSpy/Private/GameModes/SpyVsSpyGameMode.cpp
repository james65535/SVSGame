// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/SpyVsSpyGameMode.h"
#include "Rooms/RoomManager.h"
#include "UObject/ConstructorHelpers.h"

ASpyVsSpyGameMode::ASpyVsSpyGameMode()
{

}

ARoomManager* ASpyVsSpyGameMode::LoadRoomManager()
{
	ARoomManager* RoomManager = Cast<ARoomManager>(GetWorld()->SpawnActor(ARoomManager::StaticClass()));
	return RoomManager;
}
