// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "DynamicMeshRoomGen/Public/DynamicRoom.h"
#include "SVSRoom.generated.h"

class ARoomManager;
class UBoxComponent;
class ASpyCharacter; // TODO can we move this to room manager?

/**
 * Boolean Values used as floats as floats are required for Material Custom Primitive Data
 * Used to determine cartesian direction of travelling and whether coming or going
 * TTuple<float, float, float>
 * 1<Axis>: On Y Axis
 * 2<Entering/Exiting>: Exiting
 * 3<Direction on Axis: West/South
*/
USTRUCT(BlueprintType)
struct FVanishPrimitiveData
{
	GENERATED_BODY()

public:
	/** X(0.0) or Y(1.0) Axis */
	float Axis = 0.0f;
	/** Enter(0.0) or Exit(1.0) */
	float Traversal = 0.0f;
	/** North or East(0.0) or South or West(1.0) */
	float AxisDirection = 0.0f;
};

/** Begin Delegates */
/**
 * Notify listeners such as Doors and Furniture that the occupancy status of the room has changed
 * with a bool to denote of the room is occupied or not
 * 
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRoomOccupancyChange, const ASVSRoom*, const bool);

/**
 * Rooms are intended to only be visible when they are occupied by a player
 * When a room transitions from visible to invisible and vica versa,
 * a transition effect is displayed to the player in the room
 */
UCLASS()
class SPYVSSPY_API ASVSRoom : public ADynamicRoom
{
	GENERATED_BODY()
	
public:

	ASVSRoom();
	
	/** Class Overrides */
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	
	/** Material properties for Warp In / Out Effect */
	// TODO Confirm name and description
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SVS|Room")
	float VisibilityDirection = 0.0f;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SVS|Room")
	float VanishEffect = 1.0f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SVS|Room")
	bool bRoomLocallyHiddenInGame = true;

	FOnRoomOccupancyChange OnRoomOccupancyChange;

	// TODO Could update this to accomodate team play
	/**
	 * @brief Request room to hide the occupying characters or reveal them
	 * @param RequestingCharacter Pointer to Requesting Player (they will not have their visibility changed)
	 * @param bHideCharacters If Characters in room should be hidden or not
	 */
	UFUNCTION(BlueprintCallable)
	void ChangeOpposingOccupantsVisibility(const ASpyCharacter* RequestingCharacter, const bool bHideCharacters);
	
	/** Vanish Effect timeline */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Room")
	UCurveFloat* AppearTimelineCurve;
	FName AppearTimelineTrackName = "VisibilityTrack";
	FName AppearTimelinePropertyName = "AmountVisible";
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Room")
	float AppearTimelineLength = 0.5f;

	/** Manager coordinating Room activity and central point of access for Rooms */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "SVS|Room")
	ARoomManager* RoomManager;
	UFUNCTION(BlueprintCallable)
	FGuid GetRoomGuid() const { return RoomGuid; }

	UFUNCTION(BlueprintCallable, Category = "SVS|Room")
	bool IsFinalMissionRoom() const { return bIsFinalMissionRoom; }

protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category = "SVS|Room")
	bool bIsFinalMissionRoom = false;
	
private:

	/** Unique Room Identifier used by Room Manager */
	FGuid RoomGuid;

	/** Server and client RPCs to handle room overlaps and associated effects */
	UFUNCTION()
	void UnHideRoom(const ASpyCharacter* InSpyCharacter);

	/** Properties Sent to Material for Warp In / Out Effect */
	FName MPCDistanceHasEffectName = "HasEffect";
	FName MPCDistanceEffectAxisName = "EffectAxis";
	FName MPCDistanceVarName = "PlayerLoc";
	UPROPERTY(EditDefaultsOnly, Category = "SVS Room")
	UMaterialParameterCollection* WarpMPC;
	FVector WarpEffectAxis = FVector::ZeroVector;
	/** Informs the Material the direction of player travel to handle warp in / out effect */
	UFUNCTION(BlueprintCallable, Category = "SVS Room")
	FVanishPrimitiveData SetRoomTraversalDirection(const ASpyCharacter* PlayerCharacter, const bool bIsEntering) const;

	/** Timeline components for fade in / out effects */
	/** Appear */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "SVS|Room")
	UTimelineComponent* AppearTimeline;
	FOnTimelineFloat OnAppearTimelineUpdate;
	FOnTimelineEvent OnAppearTimelineFinish;
	UFUNCTION()
	void TimelineAppearUpdate(float const VisibilityInterp);
	UFUNCTION()
	void TimelineAppearFinish();

	/** Used to determine if Player Occupies Room or Not */
	UPROPERTY(VisibleAnywhere, Category = "SVS|Room")
	UBoxComponent* RoomTrigger;
	UPROPERTY(EditDefaultsOnly, Category = "SVS|Room")
	float RoomTriggerScaleMargin = 10.0f;
	UPROPERTY(EditDefaultsOnly, Category = "SVS|Room")
	float RoomTriggerHeight = 200.0f;
	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor);
	TArray<ASpyCharacter*> OccupyingSpyCharacters;
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_bRoomOccupied, Category = "SVS|Room")
	bool bRoomOccupied; // TODO determine more robust way to handle this which provides security for player view and allows for opponent view
	UFUNCTION()
	void OnRep_bRoomOccupied() {}
};
