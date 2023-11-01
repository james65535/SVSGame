// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/InteractionComponent.h"
#include "FurnitureInteractionComponent.generated.h"

class UInventoryBaseAsset;

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
	virtual UInventoryWeaponAsset* GetActiveTrap_Implementation() override;
	virtual void RemoveActiveTrap_Implementation(UInventoryWeaponAsset* InActiveTrap) override;
	virtual bool HasInventory_Implementation() override;
};
