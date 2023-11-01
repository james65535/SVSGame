// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomManager.generated.h"

class ASVSRoom;
class ADynamicRoom;
class ASpyCharacter;
struct FGuid;
/** Begin Delegates */

/**
 * Notify listeners such as a player's camera that a player as entered or exited a room
 * @input Pointer to room
 * @input Pointer to player entering / exiting
 * @input bIsEntering
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FRoomOccupiedDelegate, const ADynamicRoom*, const ASpyCharacter*, bool);

USTRUCT()
struct FRoomListing
{
	GENERATED_BODY()
public:
	ASVSRoom* Room;
	FGuid RoomGuid;
	bool bIsOccupied = false;

	FRoomListing()
	{
		Room = nullptr;
		RoomGuid = FGuid();
		bIsOccupied = false;
	}
	
	FRoomListing(ASVSRoom* InRoom, FGuid InGuid, bool bInIsOccupied)
	{
		Room = InRoom;
		RoomGuid = InGuid;
		bIsOccupied = bInIsOccupied;
	}
};

UCLASS()
class SPYVSSPY_API ARoomManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARoomManager();
	
	UFUNCTION(Server, Reliable, Category = "SVS|Room")
	void AddRoom(ASVSRoom* InDynamicRoom, const FGuid InRoomGuid);
	// TODO use array ref in case multiple characters are in the same room
	UFUNCTION(Server, Reliable, Category = "SVS|Room")
	void SetRoomOccupied(const ASVSRoom* InRoom, const bool bIsOccupied, const ASpyCharacter* PlayerCharacter);
	UFUNCTION(Blueprintable, Category = "SVS|Room")
	void GetRoomListingCollection(TArray<FRoomListing>& RoomListingCollection, const bool bGetOccupiedRooms);
	
	FRoomOccupiedDelegate OnRoomOccupied;

private:

	UPROPERTY()
	TArray<FRoomListing> RoomCollection;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
