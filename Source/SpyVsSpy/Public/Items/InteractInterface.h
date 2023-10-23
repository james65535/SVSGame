// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

class UInventoryComponent;
class UInventoryBaseAsset;
class UInventoryItemComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
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

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	bool Interact(AActor* InteractRequester);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	void ProvideInventoryListing(TArray<UInventoryBaseAsset*>& InventoryItems);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	UInventoryComponent* ProvideInventory();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SVS|Interaction")
	AActor* GetInteractableOwner();
};
