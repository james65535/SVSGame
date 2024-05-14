// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

class UInventoryComponent;
class UInventoryBaseAsset;
class UInventoryTrapAsset;
class UInventoryItemComponent;

USTRUCT(BlueprintType, Category = "DynamicRoom|Furniture")
struct FInteractableObjectInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TScriptInterface<class IInteractInterface> LatestInteractableComponentFound;
	UPROPERTY()
	bool bCanInteract;

	FInteractableObjectInfo()
	{
		LatestInteractableComponentFound = nullptr;
		bCanInteract = false;
	}

	FInteractableObjectInfo(TScriptInterface<class IInteractInterface> InComponent, const bool bInteractable)
	{
		LatestInteractableComponentFound = InComponent;
		bCanInteract = bInteractable;
	}
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for all interactions: Opening/Closing Doors, Drawers, buttons, etc...
 */
class SPYVSSPY_API IInteractInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	bool Interact(AActor* InteractRequester);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	bool HasInventory();

	// TODO probably should remove the AssetType field and just get all AssetIds
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	void GetInventoryListing(TArray<FPrimaryAssetId>& RequestedPrimaryAssetIds, const FPrimaryAssetType RequestedPrimaryAssetType);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	UInventoryComponent* GetInventory();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	AActor* GetInteractableOwner();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	UInventoryTrapAsset* GetActiveTrap();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	void RemoveActiveTrap();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	bool SetActiveTrap(UInventoryTrapAsset* InActiveTrap);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	void EnableInteractionVisualAid(const bool bEnabled);
};
