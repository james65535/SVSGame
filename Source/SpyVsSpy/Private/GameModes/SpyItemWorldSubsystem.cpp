// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/SpyItemWorldSubsystem.h"

#include "SVSLogger.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Items/InventoryBaseAsset.h"
#include "Items/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Players/SpyCharacter.h"
#include "Rooms/SpyFurniture.h"

USpyItemWorldSubsystem::USpyItemWorldSubsystem()
{

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
	UAssetManager* AssetManager = UAssetManager::GetIfValid();
	if (!IsValid(AssetManager) ||
		!InItemAssetId.IsValid() ||
		!InAssetType.IsValid())
	{ return; }
	
	/** Asset Categories to load, empty array if getting all of them */
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
	const UAssetManager* AssetManager = UAssetManager::GetIfValid();
	if (!IsValid(AssetManager) || !IsValid(AssetManager->GetPrimaryAssetObject(InItemAssetId)))
	{ return; }

	TotalItemsRequestedAndLoaded++;
	if (TotalItemsRequestedAndLoaded == TotalItemsRequested)
	{ VerifyAllItemAssetsLoaded(); }
	
	UE_LOG(SVSLogDebug, Log, TEXT(
		"Asset manager on assetload check totalitemsrequestedandloaded: %i and totalitemsrequested: %i"),
			TotalItemsRequestedAndLoaded,
			TotalItemsRequested);
}

void USpyItemWorldSubsystem::VerifyAllItemAssetsLoaded()
{
	// TODO delegate for asset manager loads, does a count check 
	const UAssetManager* AssetManager = UAssetManager::GetIfValid();
	if (!IsValid(AssetManager))
	{ return; }
	
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
	const UAssetManager* AssetManager = UAssetManager::GetIfValid();
	if (!IsValid(GetWorld()) ||
		!IsValid(GetWorld()->GetAuthGameMode<ASpyVsSpyGameMode>()) ||
		!IsValid(TargetActorClass) ||
		!IsValid(AssetManager) ||
		!ItemToDistributeAssetType.IsValid() ||
		!AllItemsVerifiedLoaded())
	{
		UE_LOG(SVSLog, Warning, TEXT("SpyItemSubsystem distribute items failed validation check"));
		return;
	}

	// TODO refactor this with proper usage of tsubclassof
	if (TargetActorClass == ASpyCharacter::StaticClass())
	{
		TArray<AActor*> WorldActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpyCharacter::StaticClass(), WorldActors);
		TArray<UObject*> AssetManagerObjectList;
		AssetManager->GetPrimaryAssetObjectList(ItemToDistributeAssetType, AssetManagerObjectList);
		/** convert object types and get primary asset ids */
		TArray<FPrimaryAssetId> InventoryBaseAssetPrimaryAssetIdCollection;
		for (UObject* AssetManagerObject : AssetManagerObjectList)
		{
			if (const UInventoryBaseAsset* AssetToAdd = Cast<UInventoryBaseAsset>(AssetManagerObject))
			{ InventoryBaseAssetPrimaryAssetIdCollection.Emplace(AssetToAdd->GetPrimaryAssetId()); }
		}

		UE_LOG(SVSLogDebug, Log, TEXT("SpyItemSubsystem distribute items found %i actors and %i items"),
			WorldActors.Num(),
			InventoryBaseAssetPrimaryAssetIdCollection.Num());
		
		for (AActor* WorldActor : WorldActors)
		{
			if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(WorldActor))
			{ SpyCharacter->GetPlayerInventoryComponent()->SetPrimaryAssetIdsToLoad(InventoryBaseAssetPrimaryAssetIdCollection); }
			else
			{
				UE_LOG(SVSLog, Warning, TEXT("SpyItemSubsystem could not find a actor for items of type: %s"),
					*ItemToDistributeAssetType.GetName().ToString());
			}
		}
	}
	else if (TargetActorClass == ASpyFurniture::StaticClass())
	{
		TArray<AActor*> WorldActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpyFurniture::StaticClass(), WorldActors);
		TArray<UObject*> AssetManagerObjectList;
		AssetManager->GetPrimaryAssetObjectList(ItemToDistributeAssetType, AssetManagerObjectList);
	
		uint8 RandomIndexMax = WorldActors.Num() - 1;
		for (UObject* AssetManagerObject : AssetManagerObjectList)
		{
			// TODO refactor to re-roll til happy
			const int32 RandomIndex = FMath::RandRange(0, RandomIndexMax); 
			const ASpyFurniture* SpyFurniture = Cast<ASpyFurniture>(WorldActors[RandomIndex]);
			const FPrimaryAssetId ObjectPrimaryAssetId = AssetManagerObject->GetPrimaryAssetId();

			const UInventoryBaseAsset* InventoryItemToAdd = Cast<UInventoryBaseAsset>(AssetManagerObject);
			if (IsValid(SpyFurniture) &&
				IsValid(SpyFurniture->GetInventoryComponent()) &&
				IsValid(InventoryItemToAdd) &&
				RandomIndex >= 0)
			{
				TArray<FPrimaryAssetId> AssetPrimaryIdsToAdd;
				AssetPrimaryIdsToAdd.Emplace(ObjectPrimaryAssetId);
				SpyFurniture->GetInventoryComponent()->SetPrimaryAssetIdsToLoad(AssetPrimaryIdsToAdd);

				/** Remove actor from candidates so we don't put all items in one basket */
				const uint8 ActorsRemovedTotal = WorldActors.RemoveSingle(WorldActors[RandomIndex]);
				if (ActorsRemovedTotal  >= 1)
				{ RandomIndexMax--;}
				else
				{
					UE_LOG(SVSLog, Warning, TEXT(
						"SpyItemSubsystem failed to remove actor from distribution array after providing it with an item"));
				}
			}
			else
			{
				UE_LOG(SVSLog, Warning, TEXT("SpyItemSubsystem could not find an actor for item: %s"),
					*AssetManagerObject->GetName())
			}
		}
	}
}
