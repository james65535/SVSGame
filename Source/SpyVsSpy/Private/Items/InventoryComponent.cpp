// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventoryComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Items/InventoryBaseAsset.h"
#include "Items/InventoryItemComponent.h"
#include "Items/InventoryWeaponAsset.h"
#include "Players/SpyCharacter.h"
#include "Players/SpyPlayerController.h"
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
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;
	SharedParams.Condition = COND_AutonomousOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryComponent, InventoryCollection, SharedParams);
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
	
	if (InventoryCollection.Num() > ArrayCountTotalBeforeEmplace)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, InventoryCollection, this);
		return true;
	}
	
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

void UInventoryComponent::OnRep_InventoryCollection() const
{
	if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetOwner()))
	{
		if (ASpyPlayerController* SpyPlayerController = SpyCharacter->GetController<ASpyPlayerController>())
		{ SpyPlayerController->C_DisplayCharacterInventory(); }
	}
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
