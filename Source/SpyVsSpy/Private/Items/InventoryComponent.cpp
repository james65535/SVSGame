// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventoryComponent.h"

#include "SVSLogger.h"
#include "Engine/AssetManager.h"
#include "Items/InventoryBaseAsset.h"
#include "Items/InventoryItemComponent.h"
#include "Items/InventoryWeaponAsset.h"
#include "UObject/PrimaryAssetId.h"
#include "GameFramework/GameModeBase.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Players/SpyCharacter.h"
#include "Players/SpyPlayerController.h"
#include "Players/SpyPlayerState.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;

	DOREPLIFETIME_WITH_PARAMS_FAST(UInventoryComponent, PrimaryAssetIdsToLoad, SharedParams);
}

void UInventoryComponent::SetPrimaryAssetIdsToLoad(TArray<FPrimaryAssetId>& InPrimaryAssetIdsToLoad)
{
	UE_LOG(SVSLogDebug, Log, TEXT(
		"InventoryComponent calling SetPrimaryAssetIdsToLoad with count: %i"),
		InPrimaryAssetIdsToLoad.Num());
	PrimaryAssetIdsToLoad.Append(InPrimaryAssetIdsToLoad);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PrimaryAssetIdsToLoad, this);

	if (IsValid(GetWorld()->GetAuthGameMode()))
	{
		for (const FPrimaryAssetId PrimaryAssetIdToLoad : PrimaryAssetIdsToLoad)
		{ LoadInventoryAssetFromAssetId(PrimaryAssetIdToLoad); }
	}
}

void UInventoryComponent::OnRep_PrimaryAssetIdsToLoad()
{
	UE_LOG(SVSLogDebug, Log, TEXT(
		"InventoryComponent running onrep with pids to load: %i"),
		PrimaryAssetIdsToLoad.Num());
	for (const FPrimaryAssetId PrimaryAssetIdToLoad : PrimaryAssetIdsToLoad)
	{ LoadInventoryAssetFromAssetId(PrimaryAssetIdToLoad); }

	/** If this load is done on a client while they are playing then display contents of inventory in UI */
	const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetOwner());
	if (IsValid(SpyCharacter) &&
		IsValid(SpyCharacter->GetController()) &&
		SpyCharacter->GetController()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		if (ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(SpyCharacter->GetController()))
		{
			UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s InventoryComponent running onrep display inv"),
				SpyCharacter->GetController()->IsLocalController() ? *FString("Local") : *FString("Remote"),
				*SpyCharacter->GetName());
			SpyPlayerController->C_DisplayCharacterInventory();
		}
	}
}

bool UInventoryComponent::AddInventoryItems(TArray<FPrimaryAssetId>& PrimaryAssetIdCollectionToLoad)
{
	UE_LOG(SVSLogDebug, Log, TEXT("InventoryComponent calling addinventoryitems"));

	if (PrimaryAssetIdsToLoad.Num() >= 1)
	{
		for (FPrimaryAssetId PrimaryAssetId : PrimaryAssetIdCollectionToLoad)
		{ LoadInventoryAssetFromAssetId(PrimaryAssetId); }
		return true;
	}
	return false;


	
	// for (UInventoryBaseAsset* ItemAsset : InventoryItemAssets)
	// {
	// 	InventoryCollection.Emplace(ItemAsset);
	// 	if (InventoryCollection.Num() <= MaxInventorySize)
	// 	{ return true; }
	// }
	// return false;
	
	// if (InventoryCollection.Num() > ArrayCountTotalBeforeEmplace)
	// {
	// 	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, InventoryCollection, this);
	// 	return true;
	// }
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
	UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent is providing inventory list"),
		*GetOwner()->GetName());
	InInventoryItems = InventoryCollection;
}

void UInventoryComponent::GetInventoryItemPrimaryAssetIdCollection(TArray<FPrimaryAssetId>& RequestedPrimaryAssetIds, const FPrimaryAssetType RequestedPrimaryAssetType) const
{
	UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent is providing inventory list PIDs"),
		*GetOwner()->GetName());
	for (const UInventoryBaseAsset* InventoryBaseAsset : InventoryCollection)
	{
		if (InventoryBaseAsset->GetPrimaryAssetId().PrimaryAssetType == RequestedPrimaryAssetType)
		{
			RequestedPrimaryAssetIds.Emplace(InventoryBaseAsset->GetPrimaryAssetId());
			UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent is providing inventory item pid: %s"),
				*GetOwner()->GetName(),
				*InventoryBaseAsset->GetPrimaryAssetId().ToString());
		}
	}
}

void UInventoryComponent::SetActiveTrap(UInventoryWeaponAsset* InActiveTrap)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Actor has been given an active trap: %s"),
		IsValid(InActiveTrap) ? *InActiveTrap->InventoryItemName.ToString() : *FString("Null Trap"));
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
	UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent calling load asset from PID: %s"),
		*GetOwner()->GetName(),
		*InInventoryAssetId.PrimaryAssetName.ToString());
	
	if (UAssetManager* AssetManager = UAssetManager::GetIfValid())
	{
		UObject* AssetManagerObject = AssetManager->GetPrimaryAssetObject(InInventoryAssetId);
		if (UInventoryBaseAsset* SpyItem = Cast<UInventoryBaseAsset>(AssetManagerObject))
		{
			InventoryCollection.Emplace(SpyItem);
			if (UInventoryWeaponAsset* SpyWeaponItem = Cast<UInventoryWeaponAsset>(SpyItem))
			{ SetActiveTrap(SpyWeaponItem); }
		}
		else
		{
			UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent tried to load asset from PID but cast failed"),
				*GetOwner()->GetName());
		}
	}
	else
	{
		UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent tried to load asset from PID but could not get asset manager"),
				*GetOwner()->GetName());
	}
}

// void UInventoryComponent::OnInventoryAssetLoad(const FPrimaryAssetId InInventoryAssetId)
// {
// 	if (const UAssetManager* AssetManager = UAssetManager::GetIfValid())
// 	{
// 		if (UInventoryBaseAsset* InventoryAsset = Cast<UInventoryBaseAsset>(AssetManager->GetPrimaryAssetObject(InInventoryAssetId)))
// 		{
// 			// TODO this might need to be moved so load can be used more generically
// 			if (UInventoryWeaponAsset* InventoryWeaponAsset = Cast<UInventoryWeaponAsset>(InventoryAsset))
// 			{
// 				if (InventoryWeaponAsset->WeaponType == EWeaponType::Trap)
// 				{ ActiveTrap = InventoryWeaponAsset; }
// 			}
// 			InventoryCollection.Emplace(InventoryAsset);
// 		}
// 	}
// }

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
