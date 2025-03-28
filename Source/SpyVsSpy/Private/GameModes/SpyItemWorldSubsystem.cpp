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
		TotalAssetsRequestedToLoadPerTypeMap.Add(
			InAssetType,
			InItemAssetIdContainer.Num());
	}
	else
	{ return; }
	
	/** Request load of items */
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
	checkfSlow(IsValid(InventoryAsset), "SpyItemWorldSubSystem: Cound not load an asset from an asset ID")
}

bool USpyItemWorldSubsystem::VerifyAllItemAssetsLoaded() const
{
	TArray<FPrimaryAssetType> FoundAssetTypesToCheck;
	TotalAssetsRequestedToLoadPerTypeMap.GetKeys(FoundAssetTypesToCheck);
	for (FPrimaryAssetType FoundAssetType : FoundAssetTypesToCheck)
	{
		TArray<UObject*> AssetManagerObjectList;
		AssetManager->GetPrimaryAssetObjectList(FoundAssetType, AssetManagerObjectList);

		const uint8 ExpectedAssetsTotal = *TotalAssetsRequestedToLoadPerTypeMap.Find(FoundAssetType);
		const uint8 FoundAssetsTotal = AssetManagerObjectList.Num();

		UE_LOG(SVSLogDebug, Warning,
			TEXT("SpyItemSubSystem: Asset verification for type %s requested %i and found %i"),
			*FoundAssetType.ToString(),
			ExpectedAssetsTotal,
			FoundAssetsTotal);

		if (ExpectedAssetsTotal > FoundAssetsTotal)
		{ return false; }
	}

	return true;
}

bool USpyItemWorldSubsystem::DistributeItems(const FPrimaryAssetType& ItemToDistributeAssetType, const TSubclassOf<AActor> TargetActorClass)
{
	/** Runs on Server Only */
	if (!IsRunningDedicatedServer() ||
		!IsValid(TargetActorClass) ||
		!ItemToDistributeAssetType.IsValid() ||
		!AllItemsVerifiedLoaded())
	{ return false; }
	
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
		
		for (AActor* WorldActor : WorldActors)
		{
			if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(WorldActor))
			{
				const bool bInventorySetIdsToLoad = SpyCharacter->
					GetPlayerInventoryComponent()->
						SetPrimaryAssetIdsToLoad(
							InventoryBaseAssetPrimaryAssetIdCollection,
							true);
				if (bInventorySetIdsToLoad == false)
				{ return false; }
			}
			else
			{
				UE_LOG(SVSLog, Warning,
					TEXT("SpyItemSubsystem could not find a actor for items of type: %s"),
					*ItemToDistributeAssetType.GetName().ToString());
				return false;
			}
		}
		return true;
	}
	if (TargetActorClass == ASpyFurniture::StaticClass())
	{
		TArray<AActor*> FurnitureWorldActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpyFurniture::StaticClass(), FurnitureWorldActors);
		TArray<UObject*> AssetManagerObjectList;
		AssetManager->GetPrimaryAssetObjectList(ItemToDistributeAssetType, AssetManagerObjectList);

		checkfSlow(
			FurnitureWorldActors.Num() >= AssetManagerObjectList.Num(),
			"SpyWorldSubsystem requires more furniture than items to distribute");
		
		for (const UObject* AssetManagerObject : AssetManagerObjectList)
		{
			const uint8 MaxTries = FurnitureWorldActors.Num();
			for (uint8 TryIndex = 0; TryIndex < MaxTries; TryIndex++)
			{
				// TODO refactor so that item placement tries other furniture if first random does not work
				const uint8 RandomIndexLimit = FurnitureWorldActors.Num() - 1;
				const int32 RandomIndex = FMath::RandRange(0, RandomIndexLimit);
				const ASpyFurniture* SpyFurniture = Cast<ASpyFurniture>(FurnitureWorldActors[RandomIndex]);
				const UFurnitureInteractionComponent* FurnitureInteractionComponent = SpyFurniture->GetInteractionComponent();

				if (IsValid(SpyFurniture) && FurnitureInteractionComponent->IsInteractionEnabled())
				{
					const FPrimaryAssetId ObjectPrimaryAssetId = AssetManagerObject->GetPrimaryAssetId();
					TArray<FPrimaryAssetId> AssetPrimaryIdsToAdd;
					AssetPrimaryIdsToAdd.AddUnique(ObjectPrimaryAssetId);
					const bool bInventorySetIdsToLoad = SpyFurniture->
						GetInventoryComponent()->
							SetPrimaryAssetIdsToLoad(
								AssetPrimaryIdsToAdd,
								true);

					if (bInventorySetIdsToLoad == false)
					{ return false; }

					break;
				}

				/** Remove actor from candidates so we don't put all items in one basket */
				const uint8 ActorsRemovedTotal = FurnitureWorldActors.RemoveSingle(FurnitureWorldActors[RandomIndex]);
				if (ActorsRemovedTotal < 1)
				{
					UE_LOG(SVSLog, Warning, TEXT(
						"SpyItemSubsystem failed to remove actor from distribution array after trying to use it"));
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool USpyItemWorldSubsystem::RelocateInventoryAssetIds(UInventoryComponent* SourceInventory,
	TArray<UInventoryComponent*>& TargetInventories, TArray<FPrimaryAssetId>& InventoryAssetIds)
{
	if (!IsValid(SourceInventory) ||
		TargetInventories.IsEmpty() ||
		InventoryAssetIds.IsEmpty())
	{
		UE_LOG(SVSLog, Warning, TEXT(
			"SpyItemSubsystem: RelocateInventoryAssetIds failed and early returned"));
		return false;
	}
	UE_LOG(SVSLogDebug, Warning,
		TEXT("SpyItemSubsystem: RelocateInventoryAssetIds has been run for character: %s"),
		*SourceInventory->GetOwner()->GetName());
	
	/** Create a collection mapping all AssetIds to relocate with Target Inventories so that
	 * we can update each inventory in one goes and thus save on network replication for
	 * the inventories */
	TMap<UInventoryComponent*, TArray<FPrimaryAssetId>> TargetInventoryAssetIdsPairing;
	for (UInventoryComponent* TargetInventory : TargetInventories)
	{
		if (IsValid(TargetInventory))
		{ TargetInventoryAssetIdsPairing.Add(TargetInventory, TArray<FPrimaryAssetId>()); }
	}
	TArray<UInventoryComponent*> InventoryPairingKeys;
	TargetInventoryAssetIdsPairing.GenerateKeyArray(InventoryPairingKeys);
	
	/** Attempt an even distribution of AssetIds across the collection of target inventories
	 * allowing for target inventories to receive multiple AssetIds if need be */
	uint8 InventoryIndex = 0;
	for (FPrimaryAssetId InventoryAssetId : InventoryAssetIds)
	{
		/** Move on to the next AssetId if this one is not valid */
		if (!InventoryAssetId.IsValid())
		{
			UE_LOG(SVSLogDebug, Warning,
				TEXT("SpyItemSubsystem found an invalid asset during RelocateInventoryAssetIds"));
			continue;
		}

		TargetInventoryAssetIdsPairing.Find(InventoryPairingKeys[InventoryIndex])->
			Add(InventoryAssetId);
		/** Increment index so that next AssetId will use the next Inventory or
		 * restart at the beginning of the TargetInventory array */
		InventoryIndex >= (InventoryPairingKeys.Num() - 1) ?
			InventoryIndex = 0 :
			InventoryIndex++;
	}

	/** Add to each target inventory the corresponding AssetIds */
	uint8 LoadErrorCount = 0;
	for (TPair<UInventoryComponent*, TArray<FPrimaryAssetId>> TargetInventory : TargetInventoryAssetIdsPairing)
	{
		const bool bDidInventoryLoad = TargetInventory.Key->SetPrimaryAssetIdsToLoad(
			TargetInventory.Value,
			true);

		if (bDidInventoryLoad == false)
		{ LoadErrorCount++; }
	}

	// Todo the following assumes it is ok remove AssetIds which were previously found to be invalid
	/** Remove AssetIds from Source */
	const bool bDidRemoveIds = SourceInventory->SetPrimaryAssetIdsToRemove(InventoryAssetIds);
	if (bDidRemoveIds == true || LoadErrorCount == 0)
	{ return true; }

	UE_LOG(SVSLog, Warning, TEXT(
		"SpyItemSubsystem RelocateAssetIds errored with Failed to remove from Source: %s and number of failed target lods: %i"),
		bDidRemoveIds ? *FString("True") : *FString("False"),
		LoadErrorCount);
	return false;
}
