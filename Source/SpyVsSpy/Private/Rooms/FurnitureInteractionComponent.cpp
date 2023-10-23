// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/FurnitureInteractionComponent.h"

#include "SVSLogger.h"
#include "Items/InventoryComponent.h"
#include "Items/WeaponComponent.h"
#include "Items/InventoryWeaponAsset.h"
#include "Players/SpyCharacter.h"
#include "Rooms/SpyFurniture.h"

bool UFurnitureInteractionComponent::Interact_Implementation(AActor* InteractRequester)
{
	Super::Interact_Implementation(InteractRequester);

	// TODO interact animation
	// TODO interact sound
	// TODO think about associating the player who placed the trap so we can track stats...
	
	/** Check owner for an inventory component */
	if (const UInventoryComponent* FurnitureInventoryComponent = GetOwner<ASpyFurniture>()->GetInventoryComponent())
	{
		/** Check for traps otherwise can we proceed with inventory listing later on */
		if (const UInventoryWeaponAsset* FoundTrap = FurnitureInventoryComponent->GetActiveTrap())
		{
			UE_LOG(SVSLogDebug, Log, TEXT("Furniture: %s  with interaction component found a trap: %s"), *GetOwner()->GetName(), *FoundTrap->GetName());

			/** Request Trap Trigger Ability on Victim */
			if (ASpyCharacter* SpyCharacterVictim = Cast<ASpyCharacter>(InteractRequester))
			{
				SpyCharacterVictim->RequestTrapTrigger();
				/** No need to process inventory as the character has just died as a victim of the trap */
				return false;
			}
		}
		else
		{
			TArray<UInventoryBaseAsset*> InventoryItems;
			FurnitureInventoryComponent->GetInventoryItems(InventoryItems);

			for (const UInventoryBaseAsset* InventoryItem : InventoryItems)
			{ UE_LOG(SVSLogDebug, Log, TEXT("Furniture: %s interaction component found inventory item: "), *InventoryItem->GetName()); }
		}

		/** Interaction Request deemed successful */
		return true;
	}

	/** No inventory component found so interaction was not successful */
	UE_LOG(SVSLogDebug, Log, TEXT("Furniture: %s did not find an inventory component during interact request"), *GetName());
	return false;
}

UInventoryComponent* UFurnitureInteractionComponent::ProvideInventory_Implementation()
{
	return GetOwner<ASpyFurniture>()->GetInventoryComponent();
}

void UFurnitureInteractionComponent::ProvideInventoryListing_Implementation(
	TArray<UInventoryBaseAsset*>& InventoryItems)
{
	if (!IsValid(GetOwner<ASpyFurniture>()) || !IsValid(GetOwner<ASpyFurniture>()->GetInventoryComponent()))
	{ return; }
	
	GetOwner<ASpyFurniture>()->GetInventoryComponent()->GetInventoryItems(InventoryItems);
}
