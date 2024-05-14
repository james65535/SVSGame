// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/FurnitureInteractionComponent.h"

#include "SVSLogger.h"
#include "Items/InventoryComponent.h"
#include "Items/InventoryTrapAsset.h"
#include "Rooms/SpyFurniture.h"

bool UFurnitureInteractionComponent::Interact_Implementation(AActor* InteractRequester)
{
	return Super::Interact_Implementation(InteractRequester);

	// TODO interact animation
	// TODO interact sound
	// TODO think about associating the player who placed the trap so we can track stats...
}

UInventoryComponent* UFurnitureInteractionComponent::GetInventory_Implementation()
{
	return GetOwner<ASpyFurniture>()->GetInventoryComponent();
}

void UFurnitureInteractionComponent::GetInventoryListing_Implementation(
	TArray<FPrimaryAssetId>& RequestedPrimaryAssetIds, const FPrimaryAssetType RequestedPrimaryAssetType)
{
	if (!IsValid(GetOwner<ASpyFurniture>()) ||
		!IsValid(GetOwner<ASpyFurniture>()->GetInventoryComponent()))
	{
		UE_LOG(SVSLog, Log,
			TEXT("Furniture: %s Interactioncomponent cannot get inventorylisting"),
			*GetOwner()->GetName());
		return;
	}
	GetOwner<ASpyFurniture>()->GetInventoryComponent()->GetInventoryItemPIDs(RequestedPrimaryAssetIds, RequestedPrimaryAssetType);
}

UInventoryTrapAsset* UFurnitureInteractionComponent::GetActiveTrap_Implementation()
{
	if (!IsValid(GetOwner<ASpyFurniture>()) ||
		!IsValid(GetOwner<ASpyFurniture>()->GetInventoryComponent()))
	{ return nullptr; }
	
	return GetOwner<ASpyFurniture>()->GetInventoryComponent()->GetActiveTrap();
}

void UFurnitureInteractionComponent::RemoveActiveTrap_Implementation()
{
	if (!IsValid(GetOwner<ASpyFurniture>()) ||
		!IsValid(GetOwner<ASpyFurniture>()->GetInventoryComponent()))
	{ return; }

	GetOwner<ASpyFurniture>()->GetInventoryComponent()->SetActiveTrap(nullptr);
}

bool UFurnitureInteractionComponent::HasInventory_Implementation()
{
	if (!IsValid(GetOwner<ASpyFurniture>()))
	{ return false; }
	
	return GetOwner<ASpyFurniture>()->GetInventoryComponent() != nullptr;
}

bool UFurnitureInteractionComponent::SetActiveTrap_Implementation(UInventoryTrapAsset* InActiveTrap)
{
	if (!IsValid(GetOwner<ASpyFurniture>()) ||
		!IsValid(GetOwner<ASpyFurniture>()->GetInventoryComponent()))
	{ return false; }

	if (InActiveTrap->InventoryOwnerType == EInventoryOwnerType::Furniture)
	{
		GetOwner<ASpyFurniture>()->GetInventoryComponent()->SetActiveTrap(InActiveTrap);
		return true;
	}
	return false;
}

void UFurnitureInteractionComponent::EnableInteractionVisualAid_Implementation(const bool bEnabled)
{
	if (IsRunningDedicatedServer())
	{ return; }
	
	if (const ASpyFurniture* SpyFurniture = GetOwner<ASpyFurniture>())
	{
		SpyFurniture->GetMesh()->SetRenderCustomDepth(bEnabled);
		SpyFurniture->GetMesh()->SetCustomDepthStencilValue(bEnabled ? 2 : 0);
	}
}


