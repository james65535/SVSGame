// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/RoomManager.h"

#include "GameFramework/GameModeBase.h"
#include "Rooms/SVSRoom.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"
#include "Players/SpyCharacter.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

// Sets default values
ARoomManager::ARoomManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void ARoomManager::GetRoomListingCollection(TArray<FRoomListing>& RoomListingCollection, const bool bGetOccupiedRooms)
{
	if (!GetWorld()->GetAuthGameMode()->IsValidLowLevelFast() || RoomCollection.Num() < 1)
	{ return; }

	for (FRoomListing RoomListing : RoomCollection)
	{
		UE_LOG(LogTemp, Warning, TEXT("Room: %s is occupied: %s"),
			*RoomListing.Room->GetName(),
			RoomListing.bIsOccupied ? *FString("True") : *FString("False"));
		if (RoomListing.bIsOccupied == bGetOccupiedRooms)
		{ RoomListingCollection.Emplace(RoomListing); }
	}
}

// Called when the game starts or when spawned
void ARoomManager::BeginPlay()
{
	Super::BeginPlay();

	/** Run only on server if network game */
	if (!HasAuthority()) { return; }
	
	UE_LOG(LogTemp, Warning, TEXT("Room Manager begin play"));

	/** Collect all Room Actor References and update them with a reference to this manager */
	TArray<AActor*> RoomActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASVSRoom::StaticClass(), RoomActors);
	for (AActor* RoomActor : RoomActors)
	{
		if (ASVSRoom* Room = Cast<ASVSRoom>(RoomActor))
		{
			AddRoom(Room, Room->GetRoomGuid());
			Room->RoomManager = this;
		}
	}
}

void ARoomManager::AddRoom_Implementation(ASVSRoom* InDynamicRoom, const FGuid InRoomGuid)
{
	if (!IsValid(InDynamicRoom) || !HasAuthority()) { return; }
	
	const FRoomListing RoomListing = FRoomListing(InDynamicRoom, InRoomGuid, false);
	RoomCollection.Emplace(RoomListing);
}

void ARoomManager::SetRoomOccupied_Implementation(const ASVSRoom* InRoom, const bool bIsOccupied, const ASpyCharacter* PlayerCharacter)
{
	if (!IsValid(InRoom) || RoomCollection.Num() == 0 || !HasAuthority()) { return; }

	if (IsValid(PlayerCharacter))
	{
		//PlayerCharacter->UpdateCameraLocation(InRoom);
	}
	OnRoomOccupied.Broadcast(InRoom, PlayerCharacter, bIsOccupied);
	
	for (int32 Index = 0; Index < RoomCollection.Num(); Index++)
	{
		if (RoomCollection[Index].Room == InRoom)
		{
			RoomCollection[Index].bIsOccupied = bIsOccupied;
			return;
		} 
	}
}
