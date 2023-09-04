// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SpyVsSpyGameMode.generated.h"

class ARoomManager;

UCLASS(minimalapi)
class ASpyVsSpyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASpyVsSpyGameMode();

	ARoomManager* LoadRoomManager();
};



