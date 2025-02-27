// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SVSRoom.h"

#include "SVSLogger.h"
#include "DynamicMeshRoomGen/Public/DynamicRoom.h"
#include "Kismet/KismetMathLibrary.h"
#include "Players/SpyCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Rooms/RoomManager.h"
#include "Rooms/SpyFurniture.h"
#include "Components/TimelineComponent.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "Kismet/KismetGuidLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"

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
	
	SetActorHiddenInGame(bRoomLocallyHiddenInGame);
	/** Configure room appear / vanish effect timeline */
	if (IsValid(AppearTimelineCurve))
	{
		OnAppearTimelineUpdate.BindUFunction(this, "TimelineAppearUpdate");
		OnAppearTimelineFinish.BindUFunction(this, "TimelineAppearFinish");
		
		AppearTimeline->AddInterpFloat(AppearTimelineCurve, OnAppearTimelineUpdate, AppearTimelinePropertyName, AppearTimelineTrackName);
		AppearTimeline->SetTimelineLength(AppearTimelineLength);
		AppearTimeline->SetTimelineFinishedFunc(OnAppearTimelineFinish);
	}
	else { UE_LOG(SVSLog, Warning, TEXT("Room Appear Timeline Curve not valid")); }
	
	/** Set initial state of the warp in/out effect */
	// TODO Check if we need to do this for each component - Walls / Floor(s)
	GetDynamicMeshComponent()->SetCustomPrimitiveDataFloat(0, VisibilityDirection);
	for (UDynamicWall* Wall : DynamicWallSet)
	{ Wall->SetCustomPrimitiveDataFloat(0, VisibilityDirection); }
	
	/** Hide furniture */
	for (AFurnitureBase* FurnitureItem : FurnitureCollection)
	{
		if (IsValid(FurnitureItem))
		{ FurnitureItem->SetActorHiddenInGame(true); }
	}

	if(HasAuthority())
	{
		/** Optimisticly Register with Room Manager, otherwise Room Manager will grab all rooms
		 * if the below AddRoom is run before the Manager is created in world */
		if (const ASpyVsSpyGameState* GameState = Cast<ASpyVsSpyGameState>(GetWorld()->GetGameState()))
		{
			RoomManager = GameState->GetRoomManager();
			if(IsValid(RoomManager))
			{ RoomManager->AddRoom(this, RoomGuid); }
			else { UE_LOG(SVSLogDebug, Log, TEXT("This Room does not have a reference to Room Manager")); }
		}
	}

	UE_LOG(SVSLogDebug, Log, TEXT("Room adding overlap delegates"));
	/** Add delegate for Room Trigger overlaps */
	OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
	OnActorEndOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapEnd);
	
}

// #if WITH_EDITOR
// void ASVSRoom::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
// {
// 	Super::PostEditChangeProperty(PropertyChangedEvent);
// 	
// 	FName const PropertyName = (PropertyChangedEvent.Property != NULL) ?
// 		PropertyChangedEvent.Property->GetFName() :
// 		NAME_None;
//
// 	/** Allow devs to toggle visibility of room in editor */
// 	FName const VanishEffectMemberName = GET_MEMBER_NAME_CHECKED(ASVSRoom, ToggleVanishEffectVisibility);
// 	if (PropertyName == VanishEffectMemberName)
// 	{
// 		UE_LOG(SVSLogDebug, Log, TEXT("prop: %s changed with member name: %s "),
// 			*PropertyName.ToString(),
// 			*VanishEffectMemberName.ToString());
//
// 		/** Update Walls */
// 		TArray<UDynamicWall*> WallSet;
// 		Execute_GetWalls(this, WallSet);
// 		UE_LOG(SVSLogDebug, Log, TEXT("num walls found: %i "),
// 			WallSet.Num());
// 		for (UDynamicWall* DynamicWall : WallSet)
// 		{
// 			if (UWorld* World = GEngine->GetWorldFromContextObject(
// 					DynamicWall,
// 					EGetWorldErrorMode::LogAndReturnNull))
// 			{
// 				UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(
// 					WarpMPC);
// 				const bool bInstanceSetParamSuccessful = Instance->SetScalarParameterValue(
// 					FName("ToggleVanishEffectVisibility"),
// 					ToggleVanishEffectVisibility);
//
// 				UE_LOG(SVSLogDebug, Log, TEXT("%s EnableVanishEffect set to: %f and was successful: %s"),
// 					*DynamicWall->GetName(),
// 					ToggleVanishEffectVisibility,
// 					bInstanceSetParamSuccessful ? *FString("True") : *FString("False"));
// 				
// 				//UKismetMaterialLibrary::SetScalarParameterValue(DynamicWall, WarpMPC, FName("ToggleVanishEffectVisibility"), ToggleVanishEffectVisibility);
// 			} else
// 			{ UE_LOG(SVSLogDebug, Log, TEXT("get world failed")); }
// 			
// 			// DynamicWall->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
// 			// DynamicWall->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);
// 			// if (IsValid(DynamicWall))
// 			// {
// 			// 	DynamicWall->SetScalarParameterValueOnMaterials(
// 			// 		FName("ToggleVanishEffectVisibility"),
// 			// 		ToggleVanishEffectVisibility);
// 			// }
// 		}
//
// 		/** Update Floor */
// 		// Execute_GetRoomFloor(this)->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
// 		// Execute_GetRoomFloor(this)->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);
// 		//Execute_GetRoomFloor(this)->SetCustomPrimitiveDataFloat(0, CustomPrimitiveData.ToggleEffectVisibility);
// 		if (IsValid(Execute_GetRoomFloor(this)))
// 		{
// 			if (UWorld* World = GEngine->GetWorldFromContextObject(Execute_GetRoomFloor(this), EGetWorldErrorMode::LogAndReturnNull))
// 			{
// 				UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(WarpMPC);
// 				const bool bInstanceSetParamSuccessful = Instance->SetScalarParameterValue(FName("ToggleVanishEffectVisibility"), ToggleVanishEffectVisibility);
// 				UE_LOG(SVSLogDebug, Log, TEXT("%s EnableVanishEffect set to: %f and was successful: %s"),
// 					*FString("Floor"),
// 					ToggleVanishEffectVisibility,
// 					bInstanceSetParamSuccessful ? *FString("True") : *FString("False"));
// 				
// 			} else
// 			{ UE_LOG(SVSLogDebug, Log, TEXT("get world failed")); }
// 		}
// 	}
// }
// #endif

void ASVSRoom::ChangeOpposingOccupantsVisibility(const ASpyCharacter* RequestingCharacter, const bool bHideCharacters)
{
	/** Since these changes are just visual on the client then avoid running on server */
	if (!IsValid(RequestingCharacter) || IsRunningDedicatedServer()) { return; }
	
	/** Look for other spies in room and adjust their visiblity occordingly */
	for (ASpyCharacter* Spy : OccupyingSpyCharacters)
	{
		if (Spy != RequestingCharacter)
		{ Spy->SetSpyHidden(bHideCharacters); }
	}
}

FVanishPrimitiveData ASVSRoom::SetRoomTraversalDirection(const ASpyCharacter* PlayerCharacter, const bool bIsEntering) const
{
	if (bIsEntering)
	{
		/** Distance to Player is used to determine cardinal direction of player entry / exit */
		const FVector DistanceToPlayerCharacter = UKismetMathLibrary::GetDirectionUnitVector(
			GetActorLocation(),
			PlayerCharacter->GetActorLocation());

		/** Entering */
		/** Determine which axis player is travelling: X or Y */
		if (UKismetMathLibrary::Abs(DistanceToPlayerCharacter.X) > UKismetMathLibrary::Abs(DistanceToPlayerCharacter.Y))
		{
			/** Is Player Entering from North or South Door */
			if(DistanceToPlayerCharacter.X > 0.0f)
			{
				/** Entering from North */
				return FVanishPrimitiveData{0.0f, 0.0f, 0.0f, ToggleVanishEffectVisibility};
			}
			/** Entering from South */
			return FVanishPrimitiveData{0.0f, 0.0f, 1.0f, ToggleVanishEffectVisibility};
		}
		/** Is Player Entering from East or West Door */
		if(DistanceToPlayerCharacter.Y > 0.0f)
		{
			/** Entering from East */
			return FVanishPrimitiveData{1.0f, 0.0f, 0.0f, ToggleVanishEffectVisibility};
		}
		/** Entering from West */
		return FVanishPrimitiveData{1.0f, 0.0f, 1.0f, ToggleVanishEffectVisibility};
	}
	
	/** Exiting */
	const FVector DistanceToPlayerCharacter = UKismetMathLibrary::GetDirectionUnitVector(
			GetActorLocation(), PlayerCharacter->GetActorLocation());

	/** Determine which axis player is travelling on X or Y */
	if (UKismetMathLibrary::Abs(DistanceToPlayerCharacter.X) > UKismetMathLibrary::Abs(DistanceToPlayerCharacter.Y))
	{
		/** Is Player Exiting through North or South Door */
		if(DistanceToPlayerCharacter.X > 0.0f)
		{
			/** Exiting through North */
			return FVanishPrimitiveData{0.0f, 1.0f, 0.0f, ToggleVanishEffectVisibility};
		}
		/** Exiting through South */
		return FVanishPrimitiveData{0.0f, 1.0f, 1.0f, ToggleVanishEffectVisibility};
	}
	/** Is Player Exiting through East or West Door */
	if(DistanceToPlayerCharacter.Y > 0.0f)
	{
		/** Exiting through East */
		return FVanishPrimitiveData{1.0f, 1.0f, 0.0f, ToggleVanishEffectVisibility};
	}
	/** Exiting through West */
	return FVanishPrimitiveData{1.0f, 1.0f, 1.0f, ToggleVanishEffectVisibility};
}

void ASVSRoom::UnHideRoom(const ASpyCharacter* InSpyCharacter)
{
	/** Unhide */
	bRoomLocallyHiddenInGame = false; // Also used in timeline finished func to make Static Meshes Visible
	SetActorHiddenInGame(bRoomLocallyHiddenInGame);
	const FVanishPrimitiveData CustomPrimitiveData = SetRoomTraversalDirection(InSpyCharacter, true);
	// UE_LOG(SVSLogDebug, Log, TEXT("Room Enter Prim Data - Axis: %f, Traversal: %f, AxisDirection: %f"), CustomPrimitiveData.Axis, CustomPrimitiveData.Traversal, CustomPrimitiveData.AxisDirection);
		
	/** Update Walls */
	TArray<UDynamicWall*> WallSet;
	Execute_GetWalls(this, WallSet);
	for (UDynamicWall* DynamicWall : WallSet)
	{
		DynamicWall->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
		DynamicWall->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);
	}

	/** Update Floor */
	Execute_GetRoomFloor(this)->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
	Execute_GetRoomFloor(this)->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);

	/** Run the Appear effect timeline */
	if (IsValid(AppearTimeline))
	{ AppearTimeline->PlayFromStart(); }
	else
	{ UE_LOG(SVSLog, Warning, TEXT("Room timeline for appear effect is null")); }



	// FVector TraceDirection = FVector(0.0f, NeighboringRoomCheckTraceLength, 0.0f);
	// const FVector RoomLocation = GetRoomLocation_Implementation() - FVector(0.0f, 0.0f, FloorHeight);
	// const FVector TraceStartLocation = RoomLocation + TraceDirection;
	// const TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes = {UEngineTypes::ConvertToObjectType(ECC_WorldDynamic)};
	// const TArray<AActor*> TraceActorsToIgnore;
	// FHitResult TraceHitResult;
	// if (UKismetSystemLibrary::SphereTraceSingle(
	// 	this,
	// 	TraceStartLocation,
	// 	RoomLocation,
	// 	TraceObjectTypes,
	// 	true,
	// 	TraceActorsToIgnore,
	// 	EDrawDebugTrace::ForDuration,
	// 	TraceHitResult,
	// 	true,
	// 	FLinearColor::Red,
	// 	FLinearColor::Green,
	// 	20.0f))
	// {}

	/** Set Room Furniture visibility */
	for (AFurnitureBase* Furniture : FurnitureCollection)
	{
		if (IsValid(Furniture))
		{ Furniture->SetActorHiddenInGame(bRoomLocallyHiddenInGame); }
	}
}

void ASVSRoom::HideRoom(const ASpyCharacter* InSpyCharacter)
{
	/** Hide Room
	 * Used by timeline finish func to hide actor and room furniture at end of vanish effect */

	/** Room Effect should not appear on other player's client */
	if (!InSpyCharacter->IsLocallyControlled()) { return; }
	
	bRoomLocallyHiddenInGame = true; // Also used in timeline finished func to make Static Meshes Visible
	const FVanishPrimitiveData CustomPrimitiveData = SetRoomTraversalDirection(InSpyCharacter, true);
	// UE_LOG(SVSLogDebug, Log, TEXT("Room Exit Prim Data - Axis: %f, Traversal: %f, AxisDirection: %f"), CustomPrimitiveData.Axis, CustomPrimitiveData.Traversal, CustomPrimitiveData.AxisDirection);
		
	GetDynamicMeshComponent()->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
	// UE_LOG(SVSLogDebug, Log, TEXT("Room Exit AxisDirection: %f"), CustomPrimitiveData.AxisDirection);
	GetDynamicMeshComponent()->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);
	// UE_LOG(SVSLogDebug, Log, TEXT("Room Exit Axis: %f"), CustomPrimitiveData.Axis);

	/** Update Walls */
	TArray<UDynamicWall*> WallSet;
	Execute_GetWalls(this, WallSet);
	for (UDynamicWall* DynamicWall : WallSet)
	{
		DynamicWall->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
		DynamicWall->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);
	}

	/** Update Floor */
	Execute_GetRoomFloor(this)->SetCustomPrimitiveDataFloat(1, CustomPrimitiveData.AxisDirection);
	Execute_GetRoomFloor(this)->SetCustomPrimitiveDataFloat(2, CustomPrimitiveData.Axis);

	/** Set Room Furniture visibility */
	for (AFurnitureBase* Furniture : FurnitureCollection)
	{
		if (IsValid(Furniture))
		{ Furniture->SetActorHiddenInGame(bRoomLocallyHiddenInGame); }
	}
	OnRoomOccupancyChange.Broadcast(this, bRoomLocallyHiddenInGame);
		
	if (IsValid(AppearTimeline))
	{ AppearTimeline->ReverseFromEnd();	}
	else
	{ UE_LOG(SVSLog, Warning, TEXT("Room timeline for disappear effect is null")); }
}

void ASVSRoom::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (GetLocalRole() == ROLE_SimulatedProxy) { return; } // TODO test below
	//if (OtherActor->GetLocalRole() == ROLE_SimulatedProxy) { return; }

	if(ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(OtherActor))
	{
		if (IsValid(RoomManager) && HasAuthority())
		{ RoomManager->SetRoomOccupied(this, true, SpyCharacter); }
		
		OccupyingSpyCharacters.Emplace(SpyCharacter);
		
		/** Run client only Unhide logic */
		if (SpyCharacter->GetLocalRole() == ROLE_AutonomousProxy)
		{ UnHideRoom(SpyCharacter); }
	}
}

void ASVSRoom::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	if(ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(OtherActor))
	{
		OccupyingSpyCharacters.Remove(SpyCharacter);
		HideRoom(SpyCharacter);
		
		if (IsValid(RoomManager) && HasAuthority())
		{ RoomManager->SetRoomOccupied(this, false, SpyCharacter); }
	}
}

void ASVSRoom::TimelineAppearUpdate(float const VisibilityInterp) const
{
	TArray<UDynamicWall*> WallSet;
	Execute_GetWalls(this, WallSet);
	
	/** Hide the Hierarchical Instanced Static Meshes which are used as Room Decorators */
	for (UDynamicWall* DynamicWall : WallSet)
	{
		// TODO refactor this for dynamic checks
		// TODO need to flip effect since walls are mirrored
		if (DynamicWall != WestWall && DynamicWall != SouthWall)
		{ DynamicWall->SetCustomPrimitiveDataFloat(0, VisibilityInterp); }
	}

	/** Apply effect to floor */
	Execute_GetRoomFloor(this)->SetCustomPrimitiveDataFloat(0, VisibilityInterp);
}

void ASVSRoom::TimelineAppearFinish()
{
	OnRoomOccupancyChange.Broadcast(this, bRoomLocallyHiddenInGame);
	SetActorHiddenInGame(bRoomLocallyHiddenInGame); // Will already be visible if timeline makes room Appear

	// TODO refactor this so that a trace determines which walls to
	// hide should camera change cardinal directions
	WestWall->SetCustomPrimitiveDataFloat(0, 0.0);
	WestWall->SetCustomPrimitiveDataFloat(1, 0);
	WestWall->SetCustomPrimitiveDataFloat(2, 0);
	SouthWall->SetCustomPrimitiveDataFloat(0, 0.0);
	SouthWall->SetCustomPrimitiveDataFloat(1, 0);
	SouthWall->SetCustomPrimitiveDataFloat(2, 0);
	
}
