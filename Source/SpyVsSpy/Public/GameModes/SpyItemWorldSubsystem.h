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
 * This class is a singleton which handles the loading/unloading of items
 * locally (for both server and clients).  The class acts as an Spy Item
 * orientated wrapper for asset manager and should be used exclusively
 * through deterministic programming approaches to keep all networked systems in sync.
 */
UCLASS()
class SPYVSSPY_API USpyItemWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/**
	 * @brief Intended to be called by each spy level to load spy item assets into asset manager
	 * @param InItemAssetIdContainer Array of Item Assets to load for map duration
	 * @param InAssetType The Type for the PrimaryAssetIds being loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|ItemAssets")
	void LoadSpyItemAssets(const TArray<FPrimaryAssetId>& InItemAssetIdContainer, const FPrimaryAssetType InAssetType);

	/**
	 * @brief Check whether the subsystem has loaded all requested items
	 * @return All requested items successfully loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|ItemAssets")
	bool AllItemsVerifiedLoaded() const { return bAllItemAssetsLoaded; }

	/**
	 * @brief Server only method to place items on furniture actors
	 * @param ItemToDistributeAssetType Item Assets to Distribute
	 * @param TargetActorClass Type class of actors to distribute items to
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|ItemAssets")
	void DistributeItems(const FPrimaryAssetType& ItemToDistributeAssetType, const TSubclassOf<AActor> TargetActorClass);

protected:

	/** Class Overrides */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

#pragma region="AssetManagerWrapper"
	UPROPERTY(VisibleAnywhere, Category = "SVS|AssetManager")
	UAssetManager* AssetManager;
	
	/** Request Load Asset from Asset Manager */
	void LoadItemAssetFromAssetId(const FPrimaryAssetId& InItemAssetId, const FPrimaryAssetType& InAssetType);

	/** Asset Manager Async Load Delegate */
	void OnItemAssetLoadFromAssetId(const FPrimaryAssetId InItemAssetId);

	// TODO remove
	//TArray<UInventoryBaseAsset*> ItemRegistry;
#pragma endregion="AssetManagerWrapper"

#pragma region="ItemLoadVerification"
	/** Final verification of all requests being loaded */
	void TryVerifyAllItemAssetsLoaded();

	// TODO maintain load success state, bool GetLoadSuccess(), run are
	// all assetsload for each load run
	// TODO Use a deinit override to cleanup values and loaded assets from asset manager
	/** Value field is the total number of assets to check of type defined by the Map Key Value */
	TMap<FPrimaryAssetType, int32> TotalAssetsRequestedToLoadPerTypeMap;
	bool bAllItemAssetsLoaded = false;
	int32 TotalItemsRequested = 0;
	int TotalObjectsRequestedForLoad;
	int32 TotalItemsRequestedAndLoaded = 0;
#pragma endregion="ItemLoadVerification"
	
	// UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "SVS|ItemAssets")
	// void NM_DistributeItem(const ASpyFurniture* InSpyFurniture, const FPrimaryAssetId& ItemToDistributePrimaryAssetId);
};
