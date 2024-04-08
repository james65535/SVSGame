// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/SpyItemWorldSubsystem.h"

#include "SVSLogger.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Items/InventoryBaseAsset.h"
#include "Items/InventoryComponent.h"
#include "Rooms/FurnitureInteractionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Players/SpyCharacter.h"
#include "Rooms/SpyFurniture.h"

void USpyItemWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	/** Setup manager references */
	AssetManager = UAssetManager::GetIfValid();
	checkf(IsValid(AssetManager), TEXT("ItemSubsystem could not find a valid Asset Manager"));
}

void USpyItemWorldSubsystem::Deinitialize()
{
	// TODO clear state AND / OR clear assets from asset manager
	Super::Deinitialize();
}

void USpyItemWorldSubsystem::LoadSpyItemAssets(const TArray<FPrimaryAssetId>& InItemAssetIdContainer, const FPrimaryAssetType InAssetType)
{
	if (InItemAssetIdContainer.Num() < 1 && !InAssetType.IsValid())
	{ return; }
	
	/** Update Associated Map so that we can do verification later on */
	if (!TotalAssetsRequestedToLoadPerTypeMap.Contains(InAssetType))
	{
		bAllItemAssetsLoaded = false;
		TotalAssetsRequestedToLoadPerTypeMap.Add(InAssetType, InItemAssetIdContainer.Num());
		TotalItemsRequested += InItemAssetIdContainer.Num();
	}
	else
	{ return; }
	
	/** Request load of items */
	// todo add switch for fireonlyonce, perhaps set this func to a bool type
	TotalObjectsRequestedForLoad = InItemAssetIdContainer.Num();
	for (FPrimaryAssetId AssetID : InItemAssetIdContainer)
	{ LoadItemAssetFromAssetId(AssetID, InAssetType); }
}

void USpyItemWorldSubsystem::LoadItemAssetFromAssetId(const FPrimaryAssetId& InItemAssetId, const FPrimaryAssetType& InAssetType)
{
	if (!InItemAssetId.IsValid() || !InAssetType.IsValid())
	{ return; }
	
	/** Asset Categories to load, use empty array to get all of them */
	TArray<FName> CategoryBundles;

	/** Async Load Delegate */
	const FStreamableDelegate AssetAsyncLoadDelegate = FStreamableDelegate::CreateUObject(
		this,
		&ThisClass::OnItemAssetLoadFromAssetId,
		InItemAssetId);

	/** Load asset with async load delegate */
	AssetManager->LoadPrimaryAsset(InItemAssetId, CategoryBundles, AssetAsyncLoadDelegate);
}

void USpyItemWorldSubsystem::OnItemAssetLoadFromAssetId(const FPrimaryAssetId InItemAssetId)
{
	UInventoryBaseAsset* InventoryAsset = Cast<UInventoryBaseAsset>(AssetManager->GetPrimaryAssetObject(InItemAssetId));
	if (!IsValid(InventoryAsset))
	{ return; }

	// TODO remove
	/** Update Item's ItemID based upon location in registry array.  Assumes asset manager will not create dupes */
	// const uint8 RegistryIndex = ItemRegistry.AddUnique(InventoryAsset);
	// InventoryAsset->ItemID = RegistryIndex;
	
	TotalItemsRequestedAndLoaded++;
	if (TotalItemsRequestedAndLoaded == TotalItemsRequested)
	{ TryVerifyAllItemAssetsLoaded(); }
	
	UE_LOG(SVSLogDebug, Log, TEXT(
		"Asset manager on assetload check totalitemsrequestedandloaded: %i and totalitemsrequested: %i"),
			TotalItemsRequestedAndLoaded,
			TotalItemsRequested);
}

void USpyItemWorldSubsystem::TryVerifyAllItemAssetsLoaded()
{
	/** Success Count per FoundAssetType, incremented if successful */
	int PerFoundAssetTypeCountCheckSuccessTotal = 0;
	
	TArray<FPrimaryAssetType> FoundAssetTypesToCheck;
	TotalAssetsRequestedToLoadPerTypeMap.GetKeys(FoundAssetTypesToCheck);
	for (FPrimaryAssetType FoundAssetType : FoundAssetTypesToCheck)
	{
		TArray<UObject*> AssetManagerObjectList;
		AssetManager->GetPrimaryAssetObjectList(FoundAssetType, AssetManagerObjectList);

		const int32 ExpectedCount = *TotalAssetsRequestedToLoadPerTypeMap.Find(FoundAssetType);
		if (AssetManagerObjectList.Num() == ExpectedCount)
		{ PerFoundAssetTypeCountCheckSuccessTotal++; }

		for (const UObject* ManagerObjectListItem : AssetManagerObjectList)
		{
			UE_LOG(SVSLogDebug, Log, TEXT("Asset manager has object: %s of class: %s with expected type: %s"),
				*ManagerObjectListItem->GetName(),
				*ManagerObjectListItem->GetClass()->GetName(),
				*FoundAssetType.ToString());
		}
	}
	
	if (TotalAssetsRequestedToLoadPerTypeMap.Num() != PerFoundAssetTypeCountCheckSuccessTotal)
	{
		UE_LOG(SVSLogDebug, Log, TEXT(
			"SpyItemSubsystem could not get a consistent counter between AssetsToLoad: %i and AssetsLoaded %i"),
			TotalAssetsRequestedToLoadPerTypeMap.Num(),
			PerFoundAssetTypeCountCheckSuccessTotal);
	}

	bAllItemAssetsLoaded = (TotalAssetsRequestedToLoadPerTypeMap.Num() == PerFoundAssetTypeCountCheckSuccessTotal);
}

void USpyItemWorldSubsystem::DistributeItems(const FPrimaryAssetType& ItemToDistributeAssetType, const TSubclassOf<AActor> TargetActorClass)
{
	/** Runs on Server Only */
	if (!IsRunningDedicatedServer() ||
		!IsValid(TargetActorClass) ||
		!ItemToDistributeAssetType.IsValid() ||
		!AllItemsVerifiedLoaded())
	{ return; }
	
	// TODO refactor this with proper usage of tsubclassof
	if (TargetActorClass == ASpyCharacter::StaticClass())
	{
		TArray<AActor*> WorldActors;
		UGameplayStatics::GetAllActorsOfClass(
			GetWorld(),
			ASpyCharacter::StaticClass(),
			WorldActors);
		TArray<UObject*> AssetManagerObjectList;
		AssetManager->GetPrimaryAssetObjectList(ItemToDistributeAssetType, AssetManagerObjectList);

		/** convert object types and get primary asset ids */
		TArray<FPrimaryAssetId> InventoryBaseAssetPrimaryAssetIdCollection;
		for (UObject* AssetManagerObject : AssetManagerObjectList)
		{
			if (const UInventoryBaseAsset* AssetToAdd = Cast<UInventoryBaseAsset>(AssetManagerObject))
			{
				InventoryBaseAssetPrimaryAssetIdCollection.AddUnique(
					AssetToAdd->GetPrimaryAssetId());
			}
		}

		UE_LOG(SVSLogDebug, Log, TEXT("SpyItemSubsystem distribute items found %i actors and %i items"),
			WorldActors.Num(),
			InventoryBaseAssetPrimaryAssetIdCollection.Num());
		
		for (AActor* WorldActor : WorldActors)
		{
			if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(WorldActor))
			{
				SpyCharacter->GetPlayerInventoryComponent()->SetPrimaryAssetIdsToLoad(
					InventoryBaseAssetPrimaryAssetIdCollection);
			}
			else
			{
				UE_LOG(SVSLog, Warning, TEXT(
					"SpyItemSubsystem could not find a actor for items of type: %s"),
					*ItemToDistributeAssetType.GetName().ToString());
			}
		}
	}
	else if (TargetActorClass == ASpyFurniture::StaticClass())
	{
		TArray<AActor*> FurnitureWorldActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpyFurniture::StaticClass(), FurnitureWorldActors);
		TArray<UObject*> AssetManagerObjectList;
		AssetManager->GetPrimaryAssetObjectList(ItemToDistributeAssetType, AssetManagerObjectList);
		
		for (const UObject* AssetManagerObject : AssetManagerObjectList)
		{
			const uint8 MaxTries = FurnitureWorldActors.Num();
			for (uint8 TryIndex = 0; TryIndex < MaxTries; TryIndex++)
			{
				const uint8 RandomIndexMax = FurnitureWorldActors.Num() - 1;
				const int32 RandomIndex = FMath::RandRange(0, RandomIndexMax);
				const ASpyFurniture* SpyFurniture = Cast<ASpyFurniture>(FurnitureWorldActors[RandomIndex]);
				const UFurnitureInteractionComponent* FurnitureInteractionComponent = SpyFurniture->GetInteractionComponent();

				if (IsValid(SpyFurniture) && FurnitureInteractionComponent->IsInteractionEnabled())
				{
					const FPrimaryAssetId ObjectPrimaryAssetId = AssetManagerObject->GetPrimaryAssetId();
					TArray<FPrimaryAssetId> AssetPrimaryIdsToAdd;
					AssetPrimaryIdsToAdd.AddUnique(ObjectPrimaryAssetId);
					SpyFurniture->GetInventoryComponent()->SetPrimaryAssetIdsToLoad(AssetPrimaryIdsToAdd);
					break;
				}

				/** Remove actor from candidates so we don't put all items in one basket */
				const uint8 ActorsRemovedTotal = FurnitureWorldActors.RemoveSingle(FurnitureWorldActors[RandomIndex]);
				if (ActorsRemovedTotal < 1)
				{
					UE_LOG(SVSLog, Warning, TEXT(
						"SpyItemSubsystem failed to remove actor from distribution array after trying to use it"));
				}
			}
		}
	}
}
