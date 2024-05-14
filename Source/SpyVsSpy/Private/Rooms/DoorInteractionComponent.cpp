// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/DoorInteractionComponent.h"

#include "SVSLogger.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Items/InventoryComponent.h"
#include "Items/InventoryTrapAsset.h"
#include "Rooms/SVSDynamicDoor.h"

// Sets default values for this component's properties
UDoorInteractionComponent::UDoorInteractionComponent()
{
	DoorState = EDoorState::Closed;
	DoorTransitionTimeline = CreateDefaultSubobject<UTimelineComponent>("Door Transition Timeline");
	TimelineDirection = ETimelineDirection::Forward;
}

void UDoorInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(DoorTransitionTimelineCurve))
	{
		OnDoorTransitionUpdate.BindUFunction(this, "DoorTransitionTimelineUpdate");
		OnDoorTransitionFinish.BindUFunction(this, "DoorTransitionTimelineFinish");

		DoorTransitionTimeline->AddInterpFloat(
			DoorTransitionTimelineCurve,
			OnDoorTransitionUpdate,
			AppearTimelinePropertyName,
			DoorTransitionTrackName);
		
		DoorTransitionTimeline->SetTimelineFinishedFunc(OnDoorTransitionFinish);
	}
	else
	{ UE_LOG(SVSLog, Warning, TEXT("Door transition timeline curve not valid")); }

	if (const UStaticMeshComponent* DoorPanel = Cast<ASVSDynamicDoor>(
		GetOwner())->GetDoorPanelMesh())
	{
		StartRotation = DoorPanel->GetRelativeRotation();
		FinalRotation = StartRotation + FRotator(0.0f, 90.0f, 0.0f);
	}
}

bool UDoorInteractionComponent::Interact_Implementation(AActor* InteractRequester)
{
	// TODO Validate If Interaction is a valid one

	switch (DoorState)
	{
		case EDoorState::Opened:
			{
				NM_CloseDoor();
				return true;
			}
		case EDoorState::Opening:
			{
				NM_CloseDoor();
				return true;
			}
		case EDoorState::Closed:
			{
				NM_OpenDoor();
				return true;
			}
		case EDoorState::Closing:
			{
				NM_OpenDoor();
				return true;
			}
		case EDoorState::Locked:
			{
				// UnlockDoor();
				return true;
			}
		case EDoorState::Disabled:
			{ return false; }
		default: return false;
	}
}

void UDoorInteractionComponent::SetInteractionEnabled(const bool bIsEnabled)
{
	Super::SetInteractionEnabled(bIsEnabled);
	
	if (bIsEnabled)
	{
		if (IsValid(GetOwner<ASVSDynamicDoor>()))
		{
			if (UBoxComponent* OwnerCollisionVolume = GetOwner<ASVSDynamicDoor>()->GetCollisionVolume_Implementation())
			{
				/** Overlap Custom Collision Channel 1: InteractChannel */
				OwnerCollisionVolume->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
				OwnerCollisionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			}
			else
			{ UE_LOG(SVSLog, Warning, TEXT("Door Interaction Component could not set interaction overlap for owner collision volume"));	}
		}
		else
		{ UE_LOG(SVSLog, Warning, TEXT("Door Interaction Component could not get owner as a svsdynamicdoor")); }
		
		DoorState = EDoorState::Closed;
		return;
	}
	DoorState = EDoorState::Disabled;
}

UInventoryTrapAsset* UDoorInteractionComponent::GetActiveTrap_Implementation()
{
	if (!IsValid(GetOwner<ASVSDynamicDoor>()) ||
	!IsValid(GetOwner<ASVSDynamicDoor>()->GetInventoryComponent()))
	{ return nullptr; }
	
	return GetOwner<ASVSDynamicDoor>()->GetInventoryComponent()->GetActiveTrap();
}

void UDoorInteractionComponent::RemoveActiveTrap_Implementation()
{
	if (!IsValid(GetOwner<ASVSDynamicDoor>()) ||
	!IsValid(GetOwner<ASVSDynamicDoor>()->GetInventoryComponent()))
	{ return; }

	GetOwner<ASVSDynamicDoor>()->GetInventoryComponent()->SetActiveTrap(nullptr);
}

bool UDoorInteractionComponent::HasInventory_Implementation()
{
	return Super::HasInventory_Implementation();
}

bool UDoorInteractionComponent::SetActiveTrap_Implementation(UInventoryTrapAsset* InActiveTrap)
{
	if (!IsValid(GetOwner<ASVSDynamicDoor>()) ||
		!IsValid(GetOwner<ASVSDynamicDoor>()->GetInventoryComponent()) ||
		!IsValid(InActiveTrap))
	{ return false; }

	if (InActiveTrap->InventoryOwnerType == EInventoryOwnerType::Door)
	{
		/** Close door when trap is set as Quality of Life feature for players */
		if (DoorState == EDoorState::Opened)
		{ Interact_Implementation(nullptr); }
		
		GetOwner<ASVSDynamicDoor>()->GetInventoryComponent()->SetActiveTrap(InActiveTrap);
		
		return true;
	}
	return false;
}

void UDoorInteractionComponent::NM_OpenDoor_Implementation()
{
	if (IsValid(DoorOpenSfx))
	{ DoorOpenSfx->Play(); }
	
	DoorState = EDoorState::Opening;
	DoorTransitionTimeline->PlayFromStart();
}

void UDoorInteractionComponent::NM_CloseDoor_Implementation()
{
	if (IsValid(DoorCloseSfx))
	{ DoorCloseSfx->Play(); }
	
	DoorState = EDoorState::Closing;
	DoorTransitionTimeline->ReverseFromEnd();
}

void UDoorInteractionComponent::DoorOpened()
{
	OnDoorOpened.Broadcast();
}

void UDoorInteractionComponent::DoorClosed()
{
	OnDoorClosed.Broadcast();
}

void UDoorInteractionComponent::TransitionDoor(float const DoorOpenedAmount)
{
	const FRotator CurrentRotation = FMath::Lerp(StartRotation,FinalRotation,DoorOpenedAmount);
	if (UStaticMeshComponent* DoorPanel = Cast<ASVSDynamicDoor>(GetOwner())->GetDoorPanelMesh())
	{ DoorPanel->SetRelativeRotation(CurrentRotation); }
}

void UDoorInteractionComponent::DoorTransitionTimelineUpdate(float const OpenAmount)
{
	TransitionDoor(OpenAmount);
}

void UDoorInteractionComponent::DoorTransitionTimelineFinish()
{
	if (const UStaticMeshComponent* DoorPanel = Cast<ASVSDynamicDoor>(GetOwner())->GetDoorPanelMesh())
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

void UDoorInteractionComponent::EnableInteractionVisualAid_Implementation(const bool bEnabled)
{
	if (IsRunningDedicatedServer())
	{ return; }
	
	if (const ASVSDynamicDoor* Door = GetOwner<ASVSDynamicDoor>())
	{
		Door->GetStaticMeshComponent()->SetRenderCustomDepth(bEnabled);
		Door->GetStaticMeshComponent()->SetCustomDepthStencilValue(bEnabled ? 2 : 0);
	}
}