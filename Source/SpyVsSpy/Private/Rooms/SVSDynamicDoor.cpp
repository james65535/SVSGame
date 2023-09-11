// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SVSDynamicDoor.h"
#include "Rooms/DoorInteractionComponent.h"
#include "net/UnrealNetwork.h"

ASVSDynamicDoor::ASVSDynamicDoor()
{
	bReplicates = true;
	DoorInteractionComponent = CreateDefaultSubobject<UDoorInteractionComponent>("Door Interaction Component");
	DoorInteractionComponent->SetIsReplicated(true);
	DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>("Door Panel");
	DoorPanel->SetupAttachment(GetRootComponent());
	
}
