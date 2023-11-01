// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/FurnitureInteractionComponent.h"

#include "SVSLogger.h"
#include "Items/InventoryComponent.h"
#include "Items/InventoryWeaponAsset.h"
#include "Rooms/SpyFurniture.h"

bool UFurnitureInteractionComponent::Interact_Implementation(AActor* InteractRequester)
{
	return Super::Interact_Implementation(InteractRequester);

	// TODO interact animation
	// TODO interact sound
	// TODO think about associating the player who placed the trap so we can track stats...
	
	/** Check owner for an inventory component */
	// if (const UInventoryComponent* FurnitureInventoryComponent = GetOwner<ASpyFurniture>()->GetInventoryComponent())
	// {
	// 	TArray<UInventoryBaseAsset*> InventoryItems;
	// 	FurnitureInventoryComponent->GetInventoryItems(InventoryItems);
	//
	// 	for (const UInventoryBaseAsset* InventoryItem : InventoryItems)
	// 	{ UE_LOG(SVSLogDebug, Log, TEXT(
	// 		"Furniture: %s interaction component found inventory item: %s"),
	// 		*GetOwner()->GetName(),
	// 		*InventoryItem->InventoryItemName.ToString()); }
	// 	
	// 	/** Interaction Request deemed successful */
	// 	return true;
	// }

	/** No inventory component found so interaction was not successful */
	// UE_LOG(SVSLogDebug, Log, TEXT(
	// 	"Furniture: %s did not find an inventory component during interact request"),
	// 	*GetOwner()->GetName());
	// return false;
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
		UE_LOG(SVSLogDebug, Log, TEXT("Furniture: %s Interactioncomponent cannot get inventorylisting"),
			*GetOwner()->GetName());
		return;
	}
	GetOwner<ASpyFurniture>()->GetInventoryComponent()->GetInventoryItemPrimaryAssetIdCollection(RequestedPrimaryAssetIds, RequestedPrimaryAssetType);
}

UInventoryWeaponAsset* UFurnitureInteractionComponent::GetActiveTrap_Implementation()
{
	if (!IsValid(GetOwner<ASpyFurniture>()) ||
		!IsValid(GetOwner<ASpyFurniture>()->GetInventoryComponent()))
	{ return nullptr; }
	
	return GetOwner<ASpyFurniture>()->GetInventoryComponent()->GetActiveTrap();
}

void UFurnitureInteractionComponent::RemoveActiveTrap_Implementation(UInventoryWeaponAsset* InActiveTrap)
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
