// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SVSDynamicDoor.h"
#include "Rooms/DoorInteractionComponent.h"
#include "net/UnrealNetwork.h"

ASVSDynamicDoor::ASVSDynamicDoor()
{
	bReplicates = true;
	DoorInteractionComponent = CreateDefaultSubobject<UDoorInteractionComponent>("Door Interaction Component");
	DoorInteractionComponent->SetIsReplicated(true);
	Door = CreateDefaultSubobject<UStaticMeshComponent>("Door");
	Door->SetupAttachment(GetRootComponent());
}

void ASVSDynamicDoor::SetEnableDoorMesh_Implementation(bool bEnabled)
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
