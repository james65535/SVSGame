// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/DoorInteractionComponent.h"

#include "SVSLogger.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameModes/SpyVsSpyGameState.h"
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
				if(CheckMissionItems(InteractRequester))
				{
					NM_OpenDoor();
					return true;
				}
				
				return false;
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

UInventoryTrapAsset* UDoorInteractionComponent::GetActiveTrap_Implementation(AActor* InteractRequester)
{
	const ASVSDynamicDoor* OwningDoor = GetOwner<ASVSDynamicDoor>();
	if (!IsValid(OwningDoor) ||
		!IsValid(OwningDoor->GetInventoryComponent()))
	{ return nullptr; }

	/** Don't trigger traps for players who have all mission items if this is a mission door */
	if (OwningDoor->IsMissionDoor() && CheckMissionItems(InteractRequester))
	{ return nullptr; }
	
	return OwningDoor->GetInventoryComponent()->GetRiggedTrapAsset();
}

void UDoorInteractionComponent::RemoveActiveTrap_Implementation()
{
	const ASVSDynamicDoor* OwningDoor = GetOwner<ASVSDynamicDoor>();
	if (!IsValid(OwningDoor) ||
		OwningDoor->IsMissionDoor() ||
		!IsValid(OwningDoor->GetInventoryComponent()))
	{ return; }

	OwningDoor->GetInventoryComponent()->SetRiggedTrapAsset(nullptr);
}

bool UDoorInteractionComponent::HasInventory_Implementation()
{
	return Super::HasInventory_Implementation();
}

bool UDoorInteractionComponent::SetActiveTrap_Implementation(UInventoryTrapAsset* InActiveTrap)
{
	const ASVSDynamicDoor* OwningDoor = GetOwner<ASVSDynamicDoor>();
	if (!IsValid(OwningDoor) ||
		OwningDoor->IsMissionDoor() ||
		!IsValid(OwningDoor->GetInventoryComponent()) ||
		!IsValid(InActiveTrap))
	{ return false; }

	if (InActiveTrap->ObjectTypeAssociation == EObjectTypeAssociation::Door)
	{
		/** Close door when trap is set as Quality of Life feature for players */
		if (DoorState == EDoorState::Opened)
		{ Interact_Implementation(nullptr); }
		
		OwningDoor->GetInventoryComponent()->SetRiggedTrapAsset(InActiveTrap);
		
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

bool UDoorInteractionComponent::CheckMissionItems(AActor* InteractingActor) const
{
	const ASpyVsSpyGameState* GameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();
	const ACharacter* InteractingCharacter = Cast<ACharacter>(InteractingActor);
	if (IsValid(GameState) && IsValid(InteractingCharacter))
	{ return GameState->HasRequiredMissionItems(InteractingCharacter);}

	return false;
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

void UDoorInteractionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ?
		PropertyChangedEvent.Property->GetFName() :
		NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, DesiredDoorState))
	{
		if (UStaticMeshComponent* DoorPanel = Cast<ASVSDynamicDoor>(GetOwner())->GetDoorPanelMesh())
		{
			switch (DesiredDoorState)
			{
			case EDoorState::Opened:
				{
					DoorPanel->SetRelativeRotation(FRotator(FRotator::ZeroRotator));
					DoorState = EDoorState::Opened;
					break;
				}
			case EDoorState::Closed:
				{
					DoorPanel->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
					DoorState = EDoorState::Closed;
					break;
				}
			case EDoorState::Locked:
				{
					DoorPanel->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
					DoorState = EDoorState::Locked;
					break;
				}
			default:
				{
					DoorPanel->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
					DoorState = EDoorState::Closed;
					break;
				}
			}
		}
	}
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
