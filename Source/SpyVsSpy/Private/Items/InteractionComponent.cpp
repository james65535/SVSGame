// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InteractionComponent.h"

#include "SVSLogger.h"
#include "GameFramework/GameModeBase.h"
#include "Items/InventoryComponent.h"
#include "Players/SpyCharacter.h"

UInteractionComponent::UInteractionComponent()
{

}

void UInteractionComponent::SetInteractionEnabled(const bool bIsEnabled)
{
	bInteractionEnabled = bIsEnabled;
}

bool UInteractionComponent::IsInteractionEnabled() const
{
	return bInteractionEnabled;
}

bool UInteractionComponent::Interact_Implementation(AActor* InteractRequester)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s was requested to interact by %s"), *GetOwner()->GetName(), *InteractRequester->GetName());
	return IsInteractionEnabled();
}

void UInteractionComponent::GetInventoryListing_Implementation(TArray<FPrimaryAssetId>& RequestedPrimaryAssetIds, const FPrimaryAssetType RequestedPrimaryAssetType)
{
	if (!GetWorld()->GetAuthGameMode()->IsValidLowLevelFast() ||
		!bInteractionEnabled)
	{ return; }
	
	if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetOwner()))
	{
		if (const UInventoryComponent* SpyInventory = SpyCharacter->GetPlayerInventoryComponent())
		{ SpyInventory->GetInventoryItemPIDs(RequestedPrimaryAssetIds, RequestedPrimaryAssetType); }
	}
}

AActor* UInteractionComponent::GetInteractableOwner_Implementation()
{
	return GetOwner();
}

UInventoryTrapAsset* UInteractionComponent::GetActiveTrap_Implementation()
{
	return nullptr;
}

void UInteractionComponent::RemoveActiveTrap_Implementation(UInventoryTrapAsset* InActiveTrap)
{
	return;
}

bool UInteractionComponent::HasInventory_Implementation()
{
	return false;
}
