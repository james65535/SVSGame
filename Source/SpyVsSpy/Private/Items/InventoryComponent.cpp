// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventoryComponent.h"

#include "SVSLogger.h"
#include "Engine/AssetManager.h"
#include "Items/InventoryBaseAsset.h"
#include "Items/InventoryItemComponent.h"
#include "Items/InventoryTrapAsset.h"
#include "UObject/PrimaryAssetId.h"
#include "GameFramework/GameModeBase.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Players/SpyCharacter.h"
#include "Players/SpyPlayerController.h"

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
	UE_LOG(SVSLogDebug, Log,
		TEXT("InventoryComponent calling SetPrimaryAssetIdsToLoad with count: %i"),
		InPrimaryAssetIdsToLoad.Num());
	
	PrimaryAssetIdsToLoad.Append(InPrimaryAssetIdsToLoad);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PrimaryAssetIdsToLoad, this);

	if (IsValid(GetWorld()->GetAuthGameMode()))
	{
		for (const FPrimaryAssetId& PrimaryAssetIdToLoad : PrimaryAssetIdsToLoad)
		{ LoadInventoryAssetFromAssetId(PrimaryAssetIdToLoad); }
	}
}

void UInventoryComponent::OnRep_PrimaryAssetIdsToLoad()
{
	UE_LOG(SVSLogDebug, Log,
		TEXT("InventoryComponent running onrep with pids to load: %i"), PrimaryAssetIdsToLoad.Num());

	for (const FPrimaryAssetId& PrimaryAssetIdToLoad : PrimaryAssetIdsToLoad)
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

void UInventoryComponent::GetInventoryItemPIDs(TArray<FPrimaryAssetId>& RequestedPIDs, const FPrimaryAssetType RequestedPrimaryAssetType) const
{
	UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent is providing inventory list PIDs"),
		*GetOwner()->GetName());
	
	for (const UInventoryBaseAsset* InventoryBaseAsset : InventoryCollection)
	{
		if (InventoryBaseAsset->GetPrimaryAssetId().PrimaryAssetType == RequestedPrimaryAssetType)
		{
			RequestedPIDs.AddUnique(InventoryBaseAsset->GetPrimaryAssetId());

			UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent is providing inventory item pid: %s"),
				*GetOwner()->GetName(), *InventoryBaseAsset->GetPrimaryAssetId().ToString());
		}
	}
}

void UInventoryComponent::SetInventoryOwnerType(const EInventoryOwnerType InInventoryOwnerType)
{
	InventoryOwnerType = InInventoryOwnerType;
}

void UInventoryComponent::SetActiveTrap(UInventoryTrapAsset* InActiveTrap)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Actor has been given an active trap: %s"),
		IsValid(InActiveTrap) ? *InActiveTrap->InventoryItemName.ToString() : *FString("Null Trap"));

	ActiveTrap = InActiveTrap;
}

void UInventoryComponent::LoadInventoryAssetFromAssetId(const FPrimaryAssetId& InInventoryAssetId)
{
	if (const UAssetManager* AssetManager = UAssetManager::GetIfValid())
	{
		UObject* AssetManagerObject = AssetManager->GetPrimaryAssetObject(InInventoryAssetId);
		if (UInventoryBaseAsset* SpyItem = Cast<UInventoryBaseAsset>(AssetManagerObject))
		{
			InventoryCollection.AddUnique(SpyItem);
			if (UInventoryTrapAsset* SpyTrapItem = Cast<UInventoryTrapAsset>(SpyItem))
			{ SetActiveTrap(SpyTrapItem);	}
		}
		else
		{
			UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s InventoryComponent tried to load asset from PID but cast failed for object: %s"),
				*GetOwner()->GetName(),
				IsValid(AssetManagerObject) ? *AssetManagerObject->GetName() : *FString("Null object"));
		}
	}
}
