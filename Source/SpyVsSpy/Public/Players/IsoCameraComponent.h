// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "IsoCameraComponent.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UIsoCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = "SVS Camera")
	void SetRoomTarget(const ASVSRoom* InRoom);

private:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "SVS Camera")
	FVector RoomLocationOffset = FVector(-768.0f, 431.0f, 718.0f);
	
};
