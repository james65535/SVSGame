// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpyItemWorldSubsystem.generated.h"


class ASpyFurniture;

USTRUCT(BlueprintType, Category = "SVS|ItemAssets")
struct FSpyItemAssetType
{
	GENERATED_BODY()

	FPrimaryAssetType ItemAssetType;

	int ItemAssetsOfTypeTotal;

	/** Constructor */
	FSpyItemAssetType(){
		ItemAssetType = FPrimaryAssetType();
		ItemAssetsOfTypeTotal = 0;
	}
};

/**
 * 
 */
UCLASS()
class SPYVSSPY_API USpyItemWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	USpyItemWorldSubsystem();
	virtual void Deinitialize() override;

	/**
	 * @brief Intended to be called by each spy level to load spy item assets into asset manager
	 * @param InItemAssetIdContainer Array of Item Assets to load for map duration
	 * @param InAssetType The Type for the PrimaryAssetIds being loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|ItemAssets")
	void LoadSpyItemAssets(const TArray<FPrimaryAssetId>& InItemAssetIdContainer, const FPrimaryAssetType InAssetType);

	UFUNCTION(BlueprintCallable, Category = "SVS|ItemAssets")
	bool AllItemsVerifiedLoaded() { return bAllItemAssetsLoaded; }

	/**
	 * @brief Server only method to place items on furniture actors
	 * @param ItemToDistributeAssetType Item Assets to Distribute
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|ItemAssets")
	void DistributeItems(const FPrimaryAssetType& ItemToDistributeAssetType);

protected:
	// TODO maintain load success state, bool GetLoadSuccess(), run are all assetsload for each load run
	// todo deinit to cleanup vals and loaded assets from asset manager
	/** Value field is the total number of assets to check of type defined by the Map Key Value */
	TMap<FPrimaryAssetType, int32> TotalAssetsRequestedToLoadPerTypeMap;
	
	//TArray<FSpyItemAssetType> AssetTypesToCheckContainer;
	int TotalObjectsRequestedForLoad;
	
	UFUNCTION()
	void LoadItemAssetFromAssetId(const FPrimaryAssetId& InItemAssetId, const FPrimaryAssetType& InAssetType);
	UFUNCTION()
	void OnItemAssetLoadFromAssetId(const FPrimaryAssetId InItemAssetId);

	UFUNCTION()
	void VerifyAllItemAssetsLoaded();

	bool bAllItemAssetsLoaded = false;
	int32 TotalItemsRequested = 0;
	int32 TotalItemsRequestedAndLoaded = 0;

	// UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "SVS|ItemAssets")
	// void NM_DistributeItem(const ASpyFurniture* InSpyFurniture, const FPrimaryAssetId& ItemToDistributePrimaryAssetId);

	
};
