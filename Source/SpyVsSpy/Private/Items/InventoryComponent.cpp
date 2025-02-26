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
#include "Items/TrapMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Players/SpyCharacter.h"
#include "Players/SpyPlayerController.h"

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	WeaponHandSocketName = "hand_rSocket";
	TrapHandSocketName = "hand_lSocket";

	DefaultEquippedItemType = EWeaponType::Club;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParamsRepAlways;
	SharedParamsRepAlways.bIsPushBased = true;
	SharedParamsRepAlways.RepNotifyCondition = REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PrimaryAssetIdsToLoad, SharedParamsRepAlways);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, EquippedItemIndex, SharedParamsRepAlways);
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

void UInventoryComponent::LoadInventoryAssetFromAssetId(const FPrimaryAssetId& InInventoryAssetId)
{
	if (const UAssetManager* AssetManager = UAssetManager::GetIfValid())
	{
		UObject* AssetManagerObject = AssetManager->GetPrimaryAssetObject(InInventoryAssetId);
		if (UInventoryBaseAsset* SpyItem = Cast<UInventoryBaseAsset>(AssetManagerObject))
		{
			const uint8 AddedItemIndex = InventoryAssetsCollection.AddUnique(SpyItem);

			/** On Server set default first weapon actor and asset to club when we add it */
			if (const UInventoryWeaponAsset* SpyWeaponItem = Cast<UInventoryWeaponAsset>(SpyItem))
			{
				const ASpyCharacter* CharacterOwner = Cast<ASpyCharacter>(GetOwner());
				if (IsValid(CharacterOwner) &&
					SpyWeaponItem->WeaponType == DefaultEquippedItemType)
				{ InventoryAssetsCollection.Swap(0, AddedItemIndex); }
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

void UInventoryComponent::SetInventoryOwnerType(const EInventoryOwnerType InInventoryOwnerType)
{
	InventoryOwnerType = InInventoryOwnerType;

	/** If Inventory OwnerType is a Player then validate
	 * inventory component has the correct socket names for trap and weapon hands */
	if (InventoryOwnerType == EInventoryOwnerType::Player)
	{
		bool bHandSocketsValidated = false;
		if (const ASpyCharacter* CharacterOwner = Cast<ASpyCharacter>( GetOwner()))
		{
			if (CharacterOwner->GetMesh()->GetSocketByName(WeaponHandSocketName) &&
				CharacterOwner->GetMesh()->GetSocketByName(TrapHandSocketName))
			{ bHandSocketsValidated = true; }
		}
		ensureAlwaysMsgf(
			bHandSocketsValidated,
			TEXT("Inventory Component could not match expected socket names for trap and weapon with owner character mesh"));
	}
}

void UInventoryComponent::EquipInventoryItem(const EItemRotationDirection ItemRotationDirection)
{
	S_EquipInventoryItem_Implementation(ItemRotationDirection);
}

void UInventoryComponent::S_EquipInventoryItem_Implementation(const EItemRotationDirection ItemRotationDirection)
{
	/* Determine whether to increment up or down the collection array */
	int8 IndexModifier = 0;
	switch (ItemRotationDirection)
	{
		case (EItemRotationDirection::Next) :
			{
				IndexModifier = 1;
				break;
			}
		case (EItemRotationDirection::Previous) :
			{
				/* We don't want to decrement below 0 */
				EquippedItemIndex > 0 ? IndexModifier = -1 : IndexModifier = 0;
				break;
			}
		case (EItemRotationDirection::Initial) :
			{ break; }
	}

	/** If initial then index is 0, otherwise adjust index up or down */
	const uint8 NewEquippedItemIndex = ItemRotationDirection == EItemRotationDirection::Initial ?
		0 :
		EquippedItemIndex + IndexModifier;

	/** Early return if index is not valid */
	if (!IsValid(GetOwner()->GetWorld()) ||
		!InventoryAssetsCollection.IsValidIndex(NewEquippedItemIndex))
	{ return; }

	/** UnEquip old weapon or trap first and early return if we cannot unequip it
	 * Skip if we're equipping the default weapon at start of game */
	if (ItemRotationDirection != EItemRotationDirection::Initial)
	{
		const bool bDidUnEquip = UnEquipCurrentItem();
		if (!bDidUnEquip)
		{ return; }
	}
	
	/** Equip Item */
	UInventoryBaseAsset* InventoryAsset = InventoryAssetsCollection[NewEquippedItemIndex];
	/** Allowed quantities can be positive or negative one for infinite */
	bool bEquipSucceeded = false;
	if (IsValid(InventoryAsset) && InventoryAsset->Quantity != 0)
	{
		/** Do nothing for traps on server as they are just cosmetic */
		if (IsValid(Cast<UInventoryTrapAsset>(InventoryAsset)))
		{ bEquipSucceeded = true; }
		else if (UInventoryWeaponAsset* WeaponAsset = Cast<UInventoryWeaponAsset>(InventoryAsset))
		{ bEquipSucceeded = EquipWeapon(WeaponAsset);	}
	}
	
	/** Update clients of the change */
	if (bEquipSucceeded)
	{
		EquippedItemAsset = InventoryAsset;
		EquippedItemIndex = NewEquippedItemIndex;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, EquippedItemIndex, this);
	}
}

void UInventoryComponent::OnRep_EquippedItemIndex()
{
	if (IsRunningDedicatedServer() ||
		!IsValid(GetOwner()) ||
		!InventoryAssetsCollection.IsValidIndex(EquippedItemIndex))
	{
		UE_LOG(SVSLog, Warning,
			TEXT("Inventory Component ran OnRep_EquippedItemIndex with index: %i but has no owner"),
			EquippedItemIndex);
		return;
	}

	/** Client only cares about unequiping a trap asset since that is only equiped on the client */
	if (Cast<UInventoryTrapAsset>(EquippedItemAsset))
	{ UnEquipCurrentItem(); }
	
	if (const UInventoryTrapAsset* TrapAsset = Cast<UInventoryTrapAsset>(InventoryAssetsCollection[EquippedItemIndex]))
	{ EquipTrap(TrapAsset); }

	EquippedItemAsset = InventoryAssetsCollection[EquippedItemIndex];
	OnEquippedUpdated.Broadcast();
}

bool UInventoryComponent::EquipTrap(const UInventoryTrapAsset* TrapAsset)
{
	/** Create the visual representation of the trap to be held by the player */
	if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetOwner()))
	{
		UTrapMeshComponent* NewHeldTrap = NewObject<UTrapMeshComponent>(
			OwnerCharacter,
			UTrapMeshComponent::StaticClass());

		if (IsValid(NewHeldTrap))
		{
			NewHeldTrap->TrapName = TrapAsset->InventoryItemName;
			NewHeldTrap->RegisterComponent();
			NewHeldTrap->SetStaticMesh(TrapAsset->TrapMesh);

			const bool bDidAttach = NewHeldTrap->AttachToComponent(
				OwnerCharacter->GetMesh(),
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				TrapHandSocketName);

			NewHeldTrap->SetRelativeTransform(TrapAsset->HeldTrapAttachTransform);

			if (bDidAttach)
			{ CurrentHeldTrap = NewHeldTrap; }
			else
			{
				NewHeldTrap->DestroyComponent();
				UE_LOG(SVSLog, Warning, TEXT("%s InventoryComponent could not attach trap visual component for asset: %s"),
					*GetOwner()->GetName(),
					*TrapAsset->InventoryItemName.ToString());
			}
			return bDidAttach;
		}
	}
	return false;
}

bool UInventoryComponent::EquipWeapon(UInventoryWeaponAsset* WeaponAsset)
{
	const ASpyCharacter* CharacterOwner = Cast<ASpyCharacter>(GetOwner());
	const TSubclassOf<AWeapon> WeaponClass = WeaponAsset->WeaponClass;
	if (!IsValid(CharacterOwner) || !IsValid(WeaponClass))
	{ return false; }
	
	AWeapon* NewWeapon = GetOwner()->GetWorld()->SpawnActorDeferred<AWeapon>(
		WeaponClass,
		FTransform::Identity,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	bool bEquipResult = false;

	if (IsValid(NewWeapon))
	{
		NewWeapon->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		bEquipResult = NewWeapon->AttachToComponent(
			CharacterOwner->GetMesh(),
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			WeaponHandSocketName);

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
	EquipInventoryItem(EItemRotationDirection::Initial);
}

bool UInventoryComponent::UnEquipCurrentItem()
{
	EquippedItemAsset = nullptr;

	/** Assumes the Spy character is either holding a weapon or a trap, never both */
	
	/** Remove weapon actor from server */
	if (IsValid(CurrentSpawnedWeapon.Get()) && IsRunningDedicatedServer())
	{ return CurrentSpawnedWeapon->Destroy(true); }
	if (IsRunningDedicatedServer())
	{ return true; } /** server only concerned with weapona actors */

	/** Remove trap visual on clients */
	if (IsValid(CurrentHeldTrap.Get()) && !IsRunningDedicatedServer())
	{
		CurrentHeldTrap->DestroyComponent();
		return true;
	}

	return false;
}

void UInventoryComponent::EnableWeaponAttackPhase(const bool bEnableAttackPhase)
{
	if (IsValid(CurrentSpawnedWeapon.Get()))
	{ CurrentSpawnedWeapon->EnableOnTickComponentSweeps(bEnableAttackPhase); }
}
