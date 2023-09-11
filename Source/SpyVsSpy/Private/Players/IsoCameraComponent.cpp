// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/IsoCameraComponent.h"

#include "Rooms/SVSRoom.h"

void UIsoCameraComponent::SetRoomTarget(const ASVSRoom* InRoom)
{
	if (!IsValid(InRoom)){ return; }

	FVector InRoomLocation = InRoom->GetRoomLocation();
	FVector NewCameraLocation = InRoomLocation + RoomLocationOffset;
	SetWorldLocation(NewCameraLocation);
}
