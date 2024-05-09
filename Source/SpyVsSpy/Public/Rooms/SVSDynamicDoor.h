// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DynamicDoor.h"
#include "SVSDynamicDoor.generated.h"

class UDoorInteractionComponent;
class ASVSRoom;
class UInventoryComponent;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API ASVSDynamicDoor : public ADynamicDoor
{
	GENERATED_BODY()

public:

	ASVSDynamicDoor();

	/** @return Provides just the door panel mesh and not the door frame mesh */
	UFUNCTION(BlueprintCallable)
	UStaticMeshComponent* GetDoorPanelMesh() const { return DoorPanel; }

	/** Enables and Disables the Door, leaving the door frame */
	virtual void SetEnableDoorMesh_Implementation(const bool bEnabled) override;

	UFUNCTION(BlueprintCallable, Category = "SVS|Furniture")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

protected:

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere,  meta = (AllowPrivateAccess))
	UDoorInteractionComponent* DoorInteractionComponent;
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Meta = (AllowPrivateAccess = "true"), Category = "SVS")
	UInventoryComponent* InventoryComponent;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess), Category = "SVS|Door")
	UStaticMeshComponent* DoorPanel;

	/** Responds to adjacent room occupancy changes by processing visibility requests */
	UFUNCTION(BlueprintCallable)
	void OnRoomOccupancyChange(const ASVSRoom* InRoomActor, const bool bIsRoomHidden );
	
};

