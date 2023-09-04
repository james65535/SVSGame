// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/DoorInteractionComponent.h"
#include "Components/AudioComponent.h"
#include "Components/TimelineComponent.h"
#include "Rooms/SVSDynamicDoor.h"

// Sets default values for this component's properties
UDoorInteractionComponent::UDoorInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	DoorState = EDoorState::Closed;
	DoorTransitionTimeline = CreateDefaultSubobject<UTimelineComponent>("Door Transition Timeline");
	TimelineDirection = ETimelineDirection::Forward;
}

void UDoorInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(DoorTransitionTimelineCurve))
	{
		//Add the float curve to the timeline and connect it to your timelines's interpolation function
		OnDoorTransitionUpdate.BindUFunction(this, "DoorTransitionTimelineUpdate");
		OnDoorTransitionFinish.BindUFunction(this, "DoorTransitionTimelineFinish");
		DoorTransitionTimeline->AddInterpFloat(
			DoorTransitionTimelineCurve, OnDoorTransitionUpdate, AppearTimelinePropertyName, DoorTransitionTrackName);
		DoorTransitionTimeline->SetTimelineFinishedFunc(OnDoorTransitionFinish);
	}
	else { UE_LOG(LogTemp, Warning, TEXT("Door transition timeline curve not valid")); }
}

bool UDoorInteractionComponent::Interact_Implementation()
{

	UE_LOG(LogTemp, Warning, TEXT("Starting door state %d"), DoorState);
	// TODO Validate If Interaction is a valid one

	switch (DoorState)
	{
		case EDoorState::Opened:
			{
				// TODO Validation Check
				CloseDoor();
				return true;
			}
		case EDoorState::Opening:
			{
				// TODO Validation Check
				CloseDoor();
				return true;
			}
		case EDoorState::Closed:
			{
				// TODO Validation Check
				OpenDoor();
				return true;
			}
		case EDoorState::Closing:
			{
				// TODO Validation Check
				OpenDoor();
				return true;
			}
		case EDoorState::Locked:
			{
				// TODO Validation Check
				// UnlockDoor();
				return true;
			}
		default: return false;
	}
}

void UDoorInteractionComponent::OpenDoor()
{
	if (IsValid(DoorOpenSfx))
	{
		DoorOpenSfx->Play();
	}
	UE_LOG(LogTemp, Warning, TEXT("Opening Door"));
	DoorState = EDoorState::Opening;
	DoorTransitionTimeline->PlayFromStart();
}

void UDoorInteractionComponent::CloseDoor()
{
	if (IsValid(DoorCloseSfx))
	{
		DoorCloseSfx->Play();
	}
	UE_LOG(LogTemp, Warning, TEXT("Closing Door"));
	DoorState = EDoorState::Closing;
	DoorTransitionTimeline->ReverseFromEnd();
}

void UDoorInteractionComponent::DoorOpened()
{
	UE_LOG(LogTemp, Warning, TEXT("Door is Open"));
	OnDoorOpened.Broadcast();
}

void UDoorInteractionComponent::DoorClosed()
{
	UE_LOG(LogTemp, Warning, TEXT("Door is Closed"));
	OnDoorClosed.Broadcast();
}

void UDoorInteractionComponent::TransitionDoor(float const DoorOpenedAmount)
{
	const FRotator CurrentRotation = FMath::Lerp(StartRotation,FinalRotation,DoorOpenedAmount);
	if (UStaticMeshComponent* DoorPanel = Cast<ASVSDynamicDoor>(GetOwner())->GetDoorPanel())
	{
		DoorPanel->SetRelativeRotation(CurrentRotation);
	}
}

void UDoorInteractionComponent::DoorTransitionTimelineUpdate(float const OpenAmount)
{
	TransitionDoor(OpenAmount);
}

void UDoorInteractionComponent::DoorTransitionTimelineFinish()
{
	if (const UStaticMeshComponent* DoorPanel = Cast<ASVSDynamicDoor>(GetOwner())->GetDoorPanel())
	{
		if (DoorPanel->GetRelativeRotation().Yaw > 0.0f)
		{
			DoorState = EDoorState::Opened;
			DoorOpened();
		}
		else
		{
			DoorState = EDoorState::Closed;
			DoorClosed();
		}
	}
}

