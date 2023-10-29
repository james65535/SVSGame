// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventoryComponent.h"

#include "SVSLogger.h"
#include "Engine/AssetManager.h"
#include "Items/InventoryBaseAsset.h"
#include "Items/InventoryItemComponent.h"
#include "Items/InventoryWeaponAsset.h"
#include "UObject/PrimaryAssetId.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{

}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.RepNotifyCondition = REPNOTIFY_OnChanged;
	//DOREPLIFETIME_WITH_PARAMS_FAST_STATIC_ARRAY(UInventoryComponent, PrimaryAssetIdsToLoad, SharedParams)

	//DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryComponent, InventoryCollection, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryComponent, PrimaryAssetIdsToLoad, SharedParams);
}

void UInventoryComponent::SetPrimaryAssetIdsToLoad(TArray<FPrimaryAssetId> InPrimaryAssetIdsToLoad)
{
	UE_LOG(SVSLogDebug, Log, TEXT(
		"InventoryComponent calling SetPrimaryAssetIdsToLoad with count: %i"),
		InPrimaryAssetIdsToLoad.Num());
	PrimaryAssetIdsToLoad = InPrimaryAssetIdsToLoad;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PrimaryAssetIdsToLoad, this);
}

void UInventoryComponent::OnRep_PrimaryAssetIdsToLoad()
{
	for (const FPrimaryAssetId PrimaryAssetIdToLoad : PrimaryAssetIdsToLoad)
	{ LoadInventoryAssetFromAssetId(PrimaryAssetIdToLoad); }
}

bool UInventoryComponent::AddInventoryItems(TArray<UInventoryBaseAsset*>& InventoryItemAssets)
{
	const uint16 ArrayCountTotalBeforeEmplace = InventoryCollection.Num();
	for (UInventoryBaseAsset* ItemAsset : InventoryItemAssets)
	{
		InventoryCollection.Emplace(ItemAsset);
		if (InventoryCollection.Num() >= MaxInventorySize)
		{ return false; }
	}
	
	// if (InventoryCollection.Num() > ArrayCountTotalBeforeEmplace)
	// {
	// 	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, InventoryCollection, this);
	// 	return true;
	// }
	//
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

void UInventoryComponent::SetActiveTrap(UInventoryWeaponAsset* InActiveTrap)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Actor has been given an active trap: %s"),
		*InActiveTrap->InventoryItemName.ToString());
	ActiveTrap = InActiveTrap;
	//MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ActiveTrap, this);
}

// void UInventoryComponent::OnRep_InventoryCollection() const
// {
// 	if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetOwner()))
// 	{
// 		if (ASpyPlayerController* SpyPlayerController = SpyCharacter->GetController<ASpyPlayerController>())
// 		{ SpyPlayerController->C_DisplayCharacterInventory(); }
// 	}
// }

void UInventoryComponent::LoadInventoryAssetFromAssetId(const FPrimaryAssetId& InInventoryAssetId)
{
	UE_LOG(SVSLogDebug, Log, TEXT("InventoryComponent calling load asset from PID"));
	
	if (UAssetManager* AssetManager = UAssetManager::GetIfValid())
	{
		UObject* AssetManagerObject = AssetManager->GetPrimaryAssetObject(InInventoryAssetId);
		if (UInventoryBaseAsset* SpyItem = Cast<UInventoryBaseAsset>(AssetManagerObject))
		{
			InventoryCollection.Emplace(SpyItem);
			if (UInventoryWeaponAsset* SpyWeaponItem = Cast<UInventoryWeaponAsset>(SpyItem))
			{ SetActiveTrap(SpyWeaponItem); }
		}
		// /** Asset Categories to load, empty array if getting all of them */
		// TArray<FName> CategoryBundles;
		//
		// /** Async Load Delegate */
		// const FStreamableDelegate AssetAsyncLoadDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnInventoryAssetLoad, InInventoryAssetId);
  //   
		// /** Load asset with async load delegate */
		// AssetManager->LoadPrimaryAsset(InInventoryAssetId, CategoryBundles, AssetAsyncLoadDelegate);
	}
}

void UInventoryComponent::OnInventoryAssetLoad(const FPrimaryAssetId InInventoryAssetId)
{
	if (const UAssetManager* AssetManager = UAssetManager::GetIfValid())
	{
		if (UInventoryBaseAsset* InventoryAsset = Cast<UInventoryBaseAsset>(AssetManager->GetPrimaryAssetObject(InInventoryAssetId)))
		{
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

// void UInventoryComponent::OnRep_ActiveTrap()
// {
// 	if (!IsValid(ActiveTrap))
// 	{
// 		UE_LOG(SVSLogDebug, Log, TEXT("Inventory has invalid activetrap after onrep called"));
// 		return;
// 	}
// 	UE_LOG(SVSLogDebug, Log, TEXT("active trap onrep has trap: %s"),
// 		*ActiveTrap->InventoryItemName.ToString());
// }
