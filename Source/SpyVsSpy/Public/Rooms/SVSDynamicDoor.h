// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DynamicDoor.h"
#include "SVSDynamicDoor.generated.h"

class UDoorInteractionComponent;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API ASVSDynamicDoor : public ADynamicDoor
{
	GENERATED_BODY()

public:

	ASVSDynamicDoor();

	UFUNCTION(BlueprintCallable, Category = "SVS Door")
	UStaticMeshComponent* GetDoorPanel() const { return DoorPanel; };

private:

	UPROPERTY(BlueprintReadWrite, EditAnywhere,  meta = (AllowPrivateAccess = "true"), Category = "SVS Door")
	UDoorInteractionComponent* DoorInteractionComponent;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly,  meta = (AllowPrivateAccess = "true"), Category = "SVS Door")
	UStaticMeshComponent* DoorPanel;
	
};
