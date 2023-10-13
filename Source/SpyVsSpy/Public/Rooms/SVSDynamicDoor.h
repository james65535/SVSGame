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

	/** @return Provides just the door mesh and not the door frame mesh */
	UFUNCTION(BlueprintCallable, Category = "SVS|Door")
	UStaticMeshComponent* GetDoorMesh() const { return Door; }

	/** Enables and Disables the Door, leaving the door frame */
	virtual void SetEnableDoorMesh_Implementation(const bool bEnabled) override;

protected:

	virtual void BeginPlay() override;
private:

	UPROPERTY(BlueprintReadWrite, EditAnywhere,  meta = (AllowPrivateAccess = "true"), Category = "SVS|Door")
	UDoorInteractionComponent* DoorInteractionComponent;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly,  meta = (AllowPrivateAccess = "true"), Category = "SVS|Door")
	UStaticMeshComponent* Door;

	/** Responds to adjacent room occupancy changes by processing visibility requests */
	UFUNCTION(BlueprintCallable, Category = "SVS|Door")
	void OnRoomOccupancyChange(const ASVSRoom* InRoomActor, const bool bIsRoomHidden );
	
};

