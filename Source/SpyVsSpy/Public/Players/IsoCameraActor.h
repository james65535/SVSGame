// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Components/TimelineComponent.h"
#include "IsoCameraActor.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API AIsoCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:
	
	AIsoCameraActor();
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable, Category = "SVS Camera")
	void SetRoomTarget(const ASVSRoom* InRoom);

	/** Camera Move to Target Timeline */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS Camera")
	UCurveFloat* MoveTimelineCurve;
	FName MoveTimelineTrackName = "MoveTrack";
	FName MoveTimelinePropertyName = "TravelDistance";
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS Camera")
	float MoveTimelineLength = 5.0f;

private:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "SVS Camera")
	FVector RoomLocationOffset = FVector(-600.0f, -600.0f, 800.0f);

	/** Populated by Timeline */
	FVector TargetRoomOffSetLocation = FVector::ZeroVector;

	/** Timeline components for moving camera to view new room */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "SVS Room")
	UTimelineComponent* MoveTimeline;
	FOnTimelineFloat OnMoveTimelineUpdate;
	FOnTimelineEvent OnMoveTimelineFinish;
	UFUNCTION()
	void TimelineMoveUpdate(float const MoveInterp);
	UFUNCTION()
	void TimelineMoveFinish();
	
};
