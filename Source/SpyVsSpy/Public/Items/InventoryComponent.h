// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UInventoryWeaponAsset;
class UWeaponComponent;
class UInventoryItemComponent;
class UInventoryBaseAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPYVSSPY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	bool AddInventoryItem(FPrimaryAssetId InInventoryItemAssetId);
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	bool RemoveInventoryItem(UInventoryItemComponent* InInventoryItem);
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void GetInventoryItems(TArray<UInventoryBaseAsset*>& InventoryItems) const;
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	UInventoryWeaponAsset* GetActiveTrap() const { return ActiveTrap; }

protected:

	const uint8 MaxInventorySize = 8;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	TArray<UInventoryBaseAsset*> InventoryCollection;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	TArray<FPrimaryAssetId> InventoryAssetIdCollection;

	UFUNCTION()
	void LoadInventoryAssetFromAssetId(const FPrimaryAssetId InInventoryAssetId);
	UFUNCTION()
	void OnInventoryAssetLoad(const FPrimaryAssetId InInventoryAssetId);

	// TODO secure UPROP settings
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	UInventoryWeaponAsset* ActiveTrap;
		
};
