// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/InteractionComponent.h"
#include "FurnitureInteractionComponent.generated.h"

class UInventoryBaseAsset;
class UInventoryTrapAsset;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UFurnitureInteractionComponent : public UInteractionComponent
{
	GENERATED_BODY()

public:

	/** Interact Interface Override */
	/** @return Success Status */
	virtual bool Interact_Implementation(AActor* InteractRequester) override;

	virtual UInventoryComponent* GetInventory_Implementation() override;
	virtual void GetInventoryListing_Implementation(TArray<FPrimaryAssetId>& RequestedPrimaryAssetIds, const FPrimaryAssetType RequestedPrimaryAssetType) override;
	virtual UInventoryTrapAsset* GetActiveTrap_Implementation() override;
	virtual void RemoveActiveTrap_Implementation() override;
	virtual bool HasInventory_Implementation() override;
	virtual bool SetActiveTrap_Implementation(UInventoryTrapAsset* InActiveTrap) override;
	virtual void EnableInteractionVisualAid_Implementation(const bool bEnabled) override;
};
