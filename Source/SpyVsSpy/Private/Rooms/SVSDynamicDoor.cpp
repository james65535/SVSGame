// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SVSDynamicDoor.h"

#include "SVSLogger.h"
#include "Rooms/DoorInteractionComponent.h"
#include "Rooms/SVSRoom.h"

ASVSDynamicDoor::ASVSDynamicDoor()
{
	bReplicates = true;
	DoorInteractionComponent = CreateDefaultSubobject<UDoorInteractionComponent>("Door Interaction Component");
	DoorInteractionComponent->SetIsReplicated(true);
	Door = CreateDefaultSubobject<UStaticMeshComponent>("Door");
	Door->SetupAttachment(GetRootComponent());
}

void ASVSDynamicDoor::BeginPlay()
{
	Super::BeginPlay();

	/** Default to hiddena and rely on room delegate broadcasts to change */
	SetActorHiddenInGame(true);
	
	/** Setup Room Occupancy Change Room Delegates */
	if (ASVSRoom* SVSRoomA = Cast<ASVSRoom>(RoomA))
	{
		SVSRoomA->OnRoomOccupancyChange.AddUObject(this, &ThisClass::OnRoomOccupancyChange);
	} else { UE_LOG(SVSLogDebug, Log, TEXT("Door Could not add occupancy delegate to room ref A.")); }
	if (ASVSRoom* SVSRoomB = Cast<ASVSRoom>(RoomB))
	{
		SVSRoomB->OnRoomOccupancyChange.AddUObject(this, &ThisClass::OnRoomOccupancyChange);
	} else { UE_LOG(SVSLogDebug, Log, TEXT("Door Could not add occupancy delegate to room ref B.")); }
}

void ASVSDynamicDoor::SetEnableDoorMesh_Implementation(const bool bEnabled)
{
	bEnableDoorMesh = bEnabled;
	ToggleDoor(Door);
	if (bEnabled)
	{
		GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);	
	} else
	{
		GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);	
	}
		
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
		if (SVSRoomA->bRoomHiddenInGame && SVSRoomB->bRoomHiddenInGame)
		{
			UE_LOG(SVSLogDebug, Log, TEXT("Door occupency delegate found both rooms hidden"));
			SetActorHiddenInGame(true);
			return;
		}
		SetActorHiddenInGame(false);
		UE_LOG(SVSLog, Warning, TEXT("Door occupency delegate triggered but at least one room is still visible"));
	}
	else
	{
		/** Rooms do not share the same visibility bool */
		UE_LOG(SVSLogDebug, Log, TEXT("Door occupency delegate triggered but neighboring rooms are not valid pointers"));
	}
}
