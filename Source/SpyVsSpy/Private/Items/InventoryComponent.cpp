// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventoryComponent.h"

#include "SVSLogger.h"
#include "Engine/AssetManager.h"
#include "Items/InventoryBaseAsset.h"
#include "Items/InventoryItemComponent.h"
#include "Items/InventoryTrapAsset.h"
#include "Items/Weapon.h"
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

	FDoRepLifetimeParams SharedParamsRepAlways;
	SharedParamsRepAlways.bIsPushBased = true;
	SharedParamsRepAlways.RepNotifyCondition = REPNOTIFY_Always;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PrimaryAssetIdsToLoad, SharedParamsRepAlways);

	FDoRepLifetimeParams SharedParamsRepChangedAutonomousOnly;
	SharedParamsRepChangedAutonomousOnly.bIsPushBased = true;
	SharedParamsRepChangedAutonomousOnly.RepNotifyCondition = REPNOTIFY_OnChanged;
	SharedParamsRepChangedAutonomousOnly.Condition = COND_AutonomousOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, EquippedItemIndex, SharedParamsRepChangedAutonomousOnly);

}

void UInventoryComponent::SetPrimaryAssetIdsToLoad(TArray<FPrimaryAssetId>& InPrimaryAssetIdsToLoad)
{
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
	for (const FPrimaryAssetId& PrimaryAssetIdToLoad : PrimaryAssetIdsToLoad)
	{ LoadInventoryAssetFromAssetId(PrimaryAssetIdToLoad); }

	/** If this load is done on a client while they are playing then display contents of inventory in UI */
	const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetOwner());
	if (IsValid(SpyCharacter) &&
		IsValid(SpyCharacter->GetController<ASpyPlayerController>()) &&
		SpyCharacter->GetController<ASpyPlayerController>()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		if (ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(SpyCharacter->GetController()))
		{ SpyPlayerController->C_DisplayCharacterInventory(); }
	}
}

bool UInventoryComponent::AddInventoryItems(TArray<FPrimaryAssetId>& PrimaryAssetIdCollectionToLoad)
{
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
	UE_LOG(SVSLogDebug, Log, TEXT("%d Actor: %s InventoryComponent is providing inventory list"),
	GetOwner()->GetLocalRole(),
	*GetOwner()->GetName());
	
	InInventoryItems = InventoryAssetsCollection;
}

void UInventoryComponent::GetInventoryItemPIDs(TArray<FPrimaryAssetId>& RequestedPIDs, const FPrimaryAssetType RequestedPrimaryAssetType) const
{
	for (const UInventoryBaseAsset* InventoryBaseAsset : InventoryAssetsCollection)
	{
		if (InventoryBaseAsset->GetPrimaryAssetId().PrimaryAssetType == RequestedPrimaryAssetType)
		{ RequestedPIDs.AddUnique(InventoryBaseAsset->GetPrimaryAssetId()); }
	}
}

void UInventoryComponent::SetInventoryOwnerType(const EInventoryOwnerType InInventoryOwnerType)
{
	InventoryOwnerType = InInventoryOwnerType;
}

void UInventoryComponent::LoadInventoryAssetFromAssetId(const FPrimaryAssetId& InInventoryAssetId)
{
	if (const UAssetManager* AssetManager = UAssetManager::GetIfValid())
	{
		UObject* AssetManagerObject = AssetManager->GetPrimaryAssetObject(InInventoryAssetId);
		if (UInventoryBaseAsset* SpyItem = Cast<UInventoryBaseAsset>(AssetManagerObject))
		{
			const uint8 AddedItemIndex = InventoryAssetsCollection.AddUnique(SpyItem);
			if (UInventoryWeaponAsset* SpyWeaponItem = Cast<UInventoryWeaponAsset>(SpyItem))
			{
				if (SpyWeaponItem->WeaponType == EWeaponType::None)
				{
					// TODO refactor this
					InventoryAssetsCollection.Swap(0, AddedItemIndex);
					if (ASpyCharacter* CharacterOwner = Cast<ASpyCharacter>(GetOwner()))
					{
						EquipWeapon(
							CharacterOwner->GetMesh(),
							CharacterOwner->GetWeaponHandSocket(),
							SpyWeaponItem);
					}
				}
			}
		}
		else
		{
			UE_LOG(SVSLog, Log, TEXT("Actor: %s InventoryComponent tried to load asset from PID but cast failed for object: %s"),
				*GetOwner()->GetName(),
				IsValid(AssetManagerObject) ? *AssetManagerObject->GetName() : *FString("Null object"));
		}
	}
}



void UInventoryComponent::EquipInventoryItem(USceneComponent* OwnerComponent, const FName& WeaponHandSocket, const FName& TrapHandSocket, const EItemRotationDirection ItemRotationDirection)
{
	S_EquipInventoryItem_Implementation(OwnerComponent, WeaponHandSocket, TrapHandSocket, ItemRotationDirection);
}

void UInventoryComponent::S_EquipInventoryItem_Implementation(USceneComponent* OwnerComponent,
                                                              const FName& WeaponHandSocket, const FName& TrapHandSocket, const EItemRotationDirection ItemRotationDirection)
{
	/* Determine wither to increment up or down the collection */
	uint8 IndexModifier = 1;
	if (ItemRotationDirection == EItemRotationDirection::Previous)
	{ IndexModifier = -1; }
	
	const uint8 NewEquippedItemIndex = EquippedItemIndex + IndexModifier;
	if (!InventoryAssetsCollection.IsValidIndex(NewEquippedItemIndex) ||
		!IsValid(OwnerComponent) ||
		!IsValid(GetOwner()->GetWorld()))
	{ return; }

	/** UnEquip old weapon first and early return if SpawnedWeapon exists but we cannot unequip it */
	UnEquipCurrentItem();

	/** Attempt to Equip New Weapon */
	UInventoryBaseAsset* InventoryAsset = InventoryAssetsCollection[NewEquippedItemIndex];
	/** Quantities can be positive or negative one for infinite */
	bool bEquipSucceeded = false;
	if (IsValid(InventoryAsset) && InventoryAsset->Quantity != 0)
	{
		if (UInventoryTrapAsset* TrapAsset = Cast<UInventoryTrapAsset>(InventoryAsset))
		{ bEquipSucceeded = EquipTrap(OwnerComponent, TrapHandSocket, TrapAsset); }
		else if (UInventoryWeaponAsset* WeaponAsset = Cast<UInventoryWeaponAsset>(InventoryAsset))
		{ bEquipSucceeded = EquipWeapon(OwnerComponent, WeaponHandSocket, WeaponAsset); }
	}

	/** Update autonomous proxy of the change */
	if (bEquipSucceeded)
	{
		EquippedItemIndex = NewEquippedItemIndex;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass,EquippedItemIndex, this);
	}
}

bool UInventoryComponent::EquipTrap(USceneComponent* OwnerComponent, const FName& HandSocket, UInventoryTrapAsset* TrapAsset)
{
	bool bEquipResult = false;
	if(IsValid(TrapAsset->TrapMesh) && !HandSocket.IsNone())
	{
		EquippedItemAsset = TrapAsset;
		bEquipResult = true;
	}

	return bEquipResult;
}

void UInventoryComponent::OnRep_EquippedItemIndex()
{
	if (InventoryAssetsCollection.Num() == 0 ||
		!InventoryAssetsCollection.IsValidIndex(EquippedItemIndex) ||
		!IsValid(GetOwner()) ||
		IsRunningDedicatedServer())
	{ return; }

	UnEquipCurrentItem();
	EquippedItemAsset = InventoryAssetsCollection[EquippedItemIndex];
	
	OnEquippedUpdated.Broadcast();
}

bool UInventoryComponent::EquipWeapon(USceneComponent* OwnerComponent, const FName& HandSocket, UInventoryWeaponAsset* WeaponAsset)
{
	const TSubclassOf<AWeapon> WeaponClass = WeaponAsset->WeaponClass;
	AWeapon* NewWeapon = GetOwner()->GetWorld()->SpawnActorDeferred<AWeapon>(
		WeaponClass,
		FTransform::Identity,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	bool bEquipResult = false;

	if (IsValid(NewWeapon) && !HandSocket.IsNone())
	{
		NewWeapon->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		bEquipResult = NewWeapon->AttachToComponent(
			OwnerComponent,
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			HandSocket);

		/** Needs to occur after attaching to character so that initial visibility of weapon can be established */
		const bool bWeaponDidLoadProps = NewWeapon->LoadWeaponPropertyValuesFromDataAsset(WeaponAsset);
		
		/** Fail if we cannot attach */
		if (!bEquipResult || !bWeaponDidLoadProps)
		{
			UE_LOG(SVSLog, Warning, TEXT("Character: %s InventoryComponent ran EquipWeapon but could not attach weapon or load its properties from the data asset"),
				*GetOwner()->GetName());

			const bool bDidDestroy = NewWeapon->Destroy();
			ensureAlwaysMsgf(
				bDidDestroy,
				TEXT("InventoryWeaponAsset could not destroy recently created weapon during Equip with a failed attachment"));

			return false;
		}
		
		CurrentSpawnedWeapon = NewWeapon;
		
		// TODO find a way to refactor this in a better manner
		/** Set the weapon to block against the opponent's character mesh Object Type,
		 * should be either channel ECC_GameTraceChannel3 or ECC_GameTraceChannel4 */
		const ECollisionChannel OwnerMeshCollisionObjectType = Cast<ACharacter>(GetOwner())->GetMesh()->GetCollisionObjectType();
		ECollisionChannel ChannelBlockingAgainst = ECollisionChannel::ECC_Vehicle; // TODO also find a better default
		if (OwnerMeshCollisionObjectType == ECC_GameTraceChannel3)
		{ChannelBlockingAgainst = ECC_GameTraceChannel4; }
		else if (OwnerMeshCollisionObjectType == ECC_GameTraceChannel4)
		{ ChannelBlockingAgainst = ECC_GameTraceChannel3; }
		
		NewWeapon->UpdateCollisionChannelResponseToBlock(
			ChannelBlockingAgainst,
			OwnerMeshCollisionObjectType);
	}
	else
	{
		UE_LOG(SVSLog, Warning,
			TEXT("Character: %s InventoryComponent ran EquipWeapon but could not Spawn weapon of Class: %s"),
			*GetOwner()->GetName(),
			*WeaponClass->GetName());
	}

	EquippedItemAsset = WeaponAsset;
	return bEquipResult;
}

void UInventoryComponent::ResetEquipped()
{
	S_ResetEquipped_Implementation();
}

void UInventoryComponent::S_ResetEquipped_Implementation()
{
	UnEquipCurrentItem();
	EquippedItemIndex = 0;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, EquippedItemIndex, this);
}

bool UInventoryComponent::UnEquipCurrentItem()
{
	EquippedItemAsset = nullptr;

	if (IsValid(CurrentSpawnedWeapon.Get()) && IsRunningDedicatedServer())
	{ return CurrentSpawnedWeapon->Destroy(); }

	return true;
}

void UInventoryComponent::EnableWeaponAttackPhase(const bool bEnableDebug)
{
	if (IsValid(CurrentSpawnedWeapon.Get()))
	{ CurrentSpawnedWeapon->EnableOnTickComponentSweeps(bEnableDebug); }
}
