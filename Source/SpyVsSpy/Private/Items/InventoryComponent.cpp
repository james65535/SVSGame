// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventoryComponent.h"

#include "SVSLogger.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Items/InventoryBaseAsset.h"
#include "Items/InventoryItemComponent.h"
#include "Items/InventoryWeaponAsset.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{

}

bool UInventoryComponent::AddInventoryItem(FPrimaryAssetId InInventoryItemAssetId)
{
	if (!InInventoryItemAssetId.IsValid() || InventoryAssetIdCollection.Num() >= MaxInventorySize)
	{
		UE_LOG(SVSLogDebug, Log, TEXT("Could not add item %s as the asset id was not valid or max inventory has been reached: %i out of %i"), *InInventoryItemAssetId.ToString(), InventoryAssetIdCollection.Num(), MaxInventorySize);
		return false;
	}
	
	// if (InventoryCollection.Emplace(InInventoryItem) >= 0) { return true; }
	
	return false;
}

bool UInventoryComponent::RemoveInventoryItem(UInventoryItemComponent* InInventoryItem)
{
	if (!IsValid(InInventoryItem)) { return false; }
	
	// TODO validation checks

	// TODO Implement
	// if(InventoryCollection.RemoveSingle(InInventoryItem) == 1)
	// {
	// 	InInventoryItem->DestroyComponent();
	// 	return true;
	// }
	return false;
}

void UInventoryComponent::GetInventoryItems(TArray<UInventoryBaseAsset*>& InInventoryItems) const
{
	InInventoryItems = InventoryCollection;
}

void UInventoryComponent::LoadInventoryAssetFromAssetId(const FPrimaryAssetId InInventoryAssetId)
{
	if (UAssetManager* AssetManager = UAssetManager::GetIfValid())
	{
		/** Asset Categories to load, empty array if getting all of them */
		TArray<FName> CategoryBundles;

		/** Async Load Delegate */
		const FStreamableDelegate AssetAsyncLoadDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnInventoryAssetLoad, InInventoryAssetId);
    
		/** Load asset with async load delegate */
		AssetManager->LoadPrimaryAsset(InInventoryAssetId, CategoryBundles, AssetAsyncLoadDelegate);
	}
}

void UInventoryComponent::OnInventoryAssetLoad(const FPrimaryAssetId InInventoryAssetId)
{
	if (const UAssetManager* AssetManager = UAssetManager::GetIfValid())
	{
		if (UInventoryBaseAsset* InventoryAsset = Cast<UInventoryBaseAsset>(AssetManager->GetPrimaryAssetObject(InInventoryAssetId)))
		{
			UE_LOG(SVSLogDebug, Log, TEXT("Loaded Inventory Asset: %s"), *InventoryAsset->InventoryItemName.ToString());

			// TODO this might need to be moved so load can be used more generically
			if (UInventoryWeaponAsset* InventoryWeaponAsset = Cast<UInventoryWeaponAsset>(InventoryAsset))
			{
				if (InventoryWeaponAsset->WeaponType == EWeaponType::Trap)
				{ ActiveTrap = InventoryWeaponAsset; }
			}
			InventoryCollection.Emplace(InventoryAsset);
		}
	}
}
