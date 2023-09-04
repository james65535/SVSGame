// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SVSDynamicDoor.h"
#include "Rooms/DoorInteractionComponent.h"

ASVSDynamicDoor::ASVSDynamicDoor()
{
	DoorInteractionComponent = CreateDefaultSubobject<UDoorInteractionComponent>("Door Interaction Component");
	DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>("Door Panel");
	DoorPanel->SetupAttachment(GetRootComponent());
	
}
