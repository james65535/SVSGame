// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SVSDynamicDoor.h"

#include "SVSLogger.h"
#include "Rooms/DoorInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Rooms/SVSRoom.h"

ASVSDynamicDoor::ASVSDynamicDoor()
{
	bReplicates = true;
	DoorInteractionComponent = CreateDefaultSubobject<UDoorInteractionComponent>(
		"Door Interaction Component");
	DoorInteractionComponent->SetIsReplicated(true);

	DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>("Door Panel Mesh");
	DoorPanel->SetupAttachment(RootComponent);
}

void ASVSDynamicDoor::BeginPlay()
{
	/** Default to hidden and rely on room delegate broadcasts to change */
	SetActorHiddenInGame(true);
	
	Super::BeginPlay();
	
	/** Setup Room Occupancy Change Delegates */
	if (ASVSRoom* SVSRoomA = Cast<ASVSRoom>(RoomA))
	{ SVSRoomA->OnRoomOccupancyChange.AddUObject(this, &ThisClass::OnRoomOccupancyChange); }
	else
	{ UE_LOG(SVSLog, Warning, TEXT("Door Could not add occupancy delegate to room ref A.")); }
	
	if (ASVSRoom* SVSRoomB = Cast<ASVSRoom>(RoomB))
	{ SVSRoomB->OnRoomOccupancyChange.AddUObject(this, &ThisClass::OnRoomOccupancyChange); }
	else
	{ UE_LOG(SVSLog, Warning, TEXT("Door Could not add occupancy delegate to room ref B.")); }
}

void ASVSDynamicDoor::SetEnableDoorMesh_Implementation(const bool bEnabled)
{
	bEnableDoorMesh = bEnabled;
	ToggleDoor(DoorPanel);

	if (IsValid(DoorInteractionComponent))
	{
		DoorInteractionComponent->SetInteractionEnabled(bEnabled);
		DoorInteractionComponent->SetIsReplicated(bEnabled);
	}
}

void ASVSDynamicDoor::OnRoomOccupancyChange(const ASVSRoom* InRoomActor, const bool bIsRoomHidden)
{
	const ASVSRoom* SVSRoomA = Cast<ASVSRoom>(RoomA);
	const ASVSRoom* SVSRoomB = Cast<ASVSRoom>(RoomB);
	if (IsValid(SVSRoomA) && IsValid(SVSRoomB))
	{
		/** Handle if both rooms hidden */
		if (SVSRoomA->IsRoomLocallyHidden() && SVSRoomB->IsRoomLocallyHidden())
		{
			SetActorHiddenInGame(true);
			return;
		}
		SetActorHiddenInGame(false);
	}
	else
	{
		UE_LOG(SVSLog, Warning, TEXT(
			"Door occupancy delegate triggered but neighboring rooms are not valid pointers"));
	}
}
