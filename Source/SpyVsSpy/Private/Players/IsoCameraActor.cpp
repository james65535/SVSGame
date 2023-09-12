// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/IsoCameraActor.h"
#include "Rooms/SVSRoom.h"
#include "Components/TimelineComponent.h"
#include "Kismet/KismetMathLibrary.h"

AIsoCameraActor::AIsoCameraActor()
{
	/** Add room appear / vanish effect timeline */
	MoveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Move Timeline Component"));
}

void AIsoCameraActor::BeginPlay()
{
	Super::BeginPlay();

	/** Configure room appear / vanish effect timeline */
	if (IsValid(MoveTimelineCurve))
	{
		OnMoveTimelineUpdate.BindUFunction(this, "TimelineMoveUpdate");
		OnMoveTimelineFinish.BindUFunction(this, "TimelineMoveFinish");
		
		MoveTimeline->AddInterpFloat(MoveTimelineCurve, OnMoveTimelineUpdate, MoveTimelinePropertyName, MoveTimelineTrackName);
		MoveTimeline->SetTimelineLength(MoveTimelineLength);
		MoveTimeline->SetTimelineFinishedFunc(OnMoveTimelineFinish);
	}
	else { UE_LOG(LogTemp, Warning, TEXT("Room Appear Timeline Curve not valid")); }
}

void AIsoCameraActor::SetRoomTarget(const ASVSRoom* InRoom)
{
	if (!IsValid(InRoom)){ return; }

	TargetRoomOffSetLocation = InRoom->Execute_GetRoomLocation(InRoom) + RoomLocationOffset;
	MoveTimeline->PlayFromStart();
}

void AIsoCameraActor::TimelineMoveUpdate(float const MoveInterp)
{
	const FVector TargetLocation = UKismetMathLibrary::VLerp(GetActorLocation(), TargetRoomOffSetLocation, MoveInterp);
	SetActorLocation(TargetLocation);
}

void AIsoCameraActor::TimelineMoveFinish()
{
	UE_LOG(LogTemp, Warning, TEXT("IsoCameraMoveComplete"));
}
