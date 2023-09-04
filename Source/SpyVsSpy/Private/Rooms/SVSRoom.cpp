// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SVSRoom.h"
#include "DynamicMeshRoomGen/Public/DynamicRoom.h"
#include "Kismet/KismetMathLibrary.h"
#include "Players/SpyCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Rooms/RoomManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameStates/NetSessionGameState.h"
#include "Kismet/KismetGuidLibrary.h"

// using ::UKismetMathLibrary; // TODO Figure this out

ASVSRoom::ASVSRoom() : ADynamicRoom()
{
	PrimaryActorTick.bCanEverTick = false;
	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Room Trigger"));
	RoomTrigger->SetupAttachment(RootComponent);
	/** Initial size of trigger box, Box will scale with room
	 * Half size for pre-scale seems to work best when scaling with room dimensions */
	RoomTrigger->SetBoxExtent(FVector(0.5f, 0.5f, 1.0f));

	/** Add room appear / vanish effect timeline */
	AppearTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Appear Timeline Component"));
}

void ASVSRoom::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	/** Scale Trigger Box to Size of Room */
	const FVector NewRoomScale = GetRoomScale_Implementation();
	const FVector NewRoomTranslation = GetActorLocation();

	/** Resize Trigger Box in case Room Scale has Changed
	 * Note: Origin for Box is from the centre */
	RoomTrigger->SetWorldTransform(FTransform(
		FRotator::ZeroRotator,
		FVector(NewRoomTranslation.X, NewRoomTranslation.Y, NewRoomTranslation.Z + RoomTriggerScaleMargin+RoomTriggerHeight/2),
		FVector(NewRoomScale.X - RoomTriggerScaleMargin, NewRoomScale.Y - RoomTriggerScaleMargin, RoomTriggerHeight)));

	/** Assign Unique ID to Room */
	RoomGuid = UKismetGuidLibrary::NewGuid();
}

void ASVSRoom::BeginPlay()
{
	Super::BeginPlay();

	SetActorHiddenInGame(RoomHiddenInGame);

	/** Configure room appear / vanish effect timeline */
	if (IsValid(AppearTimelineCurve))
	{
		OnAppearTimelineUpdate.BindUFunction(this, "TimelineAppearUpdate");
		OnAppearTimelineFinish.BindUFunction(this, "TimelineAppearFinish");
		
		AppearTimeline->AddInterpFloat(AppearTimelineCurve, OnAppearTimelineUpdate, AppearTimelinePropertyName, AppearTimelineTrackName);
		AppearTimeline->SetTimelineLength(AppearTimelineLength);
		AppearTimeline->SetTimelineFinishedFunc(OnAppearTimelineFinish);
	}
	else { UE_LOG(LogTemp, Warning, TEXT("Room Appear Timeline Curve not valid")); }

	/** Add delegate for Room Trigger overlaps */
	OnActorBeginOverlap.AddUniqueDynamic(this, &ASVSRoom::OnOverlapBegin);
	OnActorEndOverlap.AddUniqueDynamic(this, &ASVSRoom::OnOverlapEnd);

	/** Set initial state of the warp in/out effect */
	// TODO Check if we need to do this for each component - Walls / Floor(s)
	GetDynamicMeshComponent()->SetCustomPrimitiveDataFloat(0, VisibilityDirection);
	for (UDynamicWall* Wall : DynamicWallSet)
	{
		Wall->SetCustomPrimitiveDataFloat(0, VisibilityDirection);
	}
	
	/** Hide furniture */
	for (AStaticMeshActor* FurnitureItem : FurnitureCollection)
	{
		FurnitureItem->SetActorHiddenInGame(true);
	}

	/** Optimisticly Register with Room Manager, otherwise Room Manager will grab all rooms
	 * if the below AddRoom is run before the Manager is created in world */
	if (const ANetSessionGameState* GameState = Cast<ANetSessionGameState>(GetWorld()->GetGameState()))
	{
		RoomManager = GameState->GetRoomManager();
		if(IsValid(RoomManager))
		{
			RoomManager->AddRoom(this, RoomGuid);
		}
		else { UE_LOG(LogTemp, Warning, TEXT("Room does not have a reference to Room Manager")); }
	}
}

FVanishPrimitiveData ASVSRoom::SetRoomTraversalDirection(const ASpyCharacter* PlayerCharacter, const bool bIsEntering) const
{
	if (bIsEntering)
	{
		/** Distance to Player is used to determine cardinal direction of player entry / exit */
		const FVector DistanceToPlayerCharacter = UKismetMathLibrary::GetDirectionUnitVector(
			GetActorLocation(), PlayerCharacter->GetActorLocation());

		/** Entering */
		/** Determine which axis player is travelling: X or Y */
		if (UKismetMathLibrary::Abs(DistanceToPlayerCharacter.X) > UKismetMathLibrary::Abs(DistanceToPlayerCharacter.X))
		{
			/** Is Player Entering from North or South Door */
			if(DistanceToPlayerCharacter.X > 0.0f)
			{
				/** Entering from North */
				return FVanishPrimitiveData{0.0f, 0.0f, 0.0f};
			}
			/** Entering from South */
			return FVanishPrimitiveData{0.0f, 0.0f, 1.0f};
		}
		/** Is Player Entering from East or West Door */
		if(DistanceToPlayerCharacter.Y > 0.0f)
		{
			/** Entering from East */
			return FVanishPrimitiveData{1.0f, 0.0f, 0.0f};
		}
		/** Entering from West */
		return FVanishPrimitiveData{1.0f, 0.0f, 1.0f};
	}
	
	/** Exiting */
	const FVector DistanceToPlayerCharacter = UKismetMathLibrary::GetDirectionUnitVector(
			GetActorLocation(), PlayerCharacter->GetActorLocation());

	/** Determine which axis player is travelling on X or Y */
	if (UKismetMathLibrary::Abs(DistanceToPlayerCharacter.X) > UKismetMathLibrary::Abs(DistanceToPlayerCharacter.X))
	{
		/** Is Player Exiting through North or South Door */
		if(DistanceToPlayerCharacter.X > 0.0f)
		{
			/** Exiting through North */
			return FVanishPrimitiveData{0.0f, 1.0f, 0.0f};
		}
		/** Exiting through South */
		return FVanishPrimitiveData{0.0f, 1.0f, 1.0f};
	}
	/** Is Player Exiting through East or West Door */
	if(DistanceToPlayerCharacter.Y > 0.0f)
	{
		/** Exiting through East */
		return FVanishPrimitiveData{1.0f, 1.0f, 0.0f};
	}
	/** Exiting through West */
	return FVanishPrimitiveData{1.0f, 1.0f, 1.0f};
}

void ASVSRoom::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if(const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(OtherActor))
	{
		/** Unhide
		 * Used by timeline finish func to hide actor at end of vanish effect */
		RoomHiddenInGame = false; // Also used in timeline finished func to make Static Meshes Visible
		SetActorHiddenInGame(RoomHiddenInGame);
		const FVanishPrimitiveData CustomPrimitiveData = SetRoomTraversalDirection(SpyCharacter, true);
		GetDynamicMeshComponent()->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
		UE_LOG(LogTemp, Warning, TEXT("Room Enter AxisDirection: %f"), CustomPrimitiveData.AxisDirection);
		GetDynamicMeshComponent()->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);
		UE_LOG(LogTemp, Warning, TEXT("Room Enter Axis: %f"), CustomPrimitiveData.Axis);

		/** Update Walls */
		TArray<UDynamicWall*> WallSet;
		Execute_GetWalls(this, WallSet);
		
		for (UDynamicWall* DynamicWall : WallSet)
		{
			DynamicWall->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
			DynamicWall->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);
			UE_LOG(LogTemp, Warning, TEXT("Updating wall: %s"), *DynamicWall->GetName());
		}

		/** Notify Room Manager that room is occupied */
		if (IsValid(RoomManager))
		{
			RoomManager->SetRoomOccupied(this, true, SpyCharacter);
		}
		else { UE_LOG(LogTemp, Warning, TEXT("Room could not update Room Manager when player entered")); }

		/** Run the Appear effect timeline */
		if (IsValid(AppearTimeline))
		{
			AppearTimeline->PlayFromStart();	
		}
		else { UE_LOG(LogTemp, Warning, TEXT("Room timeline for appear effect is null")); }
	}
}

void ASVSRoom::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	if(const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(OtherActor))
	{
		/** Used by timeline finish func to hide actor and room furniture at end of vanish effect */
		RoomHiddenInGame = true;
		
		if (IsValid(AppearTimeline))
		{
			AppearTimeline->ReverseFromEnd();	
		}
		else { UE_LOG(LogTemp, Warning, TEXT("Room timeline for appear effect is null")); }

		if (IsValid(RoomManager))
		{
			RoomManager->SetRoomOccupied(this, false, nullptr);
		}
		else { UE_LOG(LogTemp, Warning, TEXT("Room could not update Room Manager when player exited")); }
	}
}

void ASVSRoom::TimelineAppearUpdate(float const VisibilityInterp)
{
	UE_LOG(LogTemp, Warning, TEXT("Timeline Update Running"));
	TArray<UDynamicWall*> WallSet;
	Execute_GetWalls(this, WallSet);
	
	/** Hide the Hierarchical Instanced Static Meshes which are used as Room Decorators */
	for (UDynamicWall* DynamicWall : WallSet)
	{
		// TODO need to flip effect since walls are mirrored
		DynamicWall->SetCustomPrimitiveDataFloat(0, VisibilityInterp);
		UE_LOG(LogTemp, Warning, TEXT("Timeline Updating wall: %s with: %f"), *DynamicWall->GetName(), VisibilityInterp);
	}
	GetDynamicMeshComponent()->SetCustomPrimitiveDataFloat(0, VisibilityInterp);
}

void ASVSRoom::TimelineAppearFinish()
{
	SetActorHiddenInGame(RoomHiddenInGame); // Will already be visible if timeline makes room Appear
	
	/** Set Room Furniture visibility */
	for (AStaticMeshActor* Furniture : FurnitureCollection)
	{
		Furniture->SetActorHiddenInGame(RoomHiddenInGame);
	}
}





