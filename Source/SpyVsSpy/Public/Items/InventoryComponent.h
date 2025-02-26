// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

enum class EWeaponType : uint8;
class UTrapMeshComponent;
class AStaticMeshActor;
class UInventoryWeaponAsset;
class UInventoryTrapAsset;
class UInventoryItemComponent;
class UInventoryBaseAsset;
class AWeapon;

DECLARE_MULTICAST_DELEGATE(FOnInventoryUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquippedUpdated);

UENUM(BlueprintType)
enum class EInventoryOwnerType : uint8
{
	None			UMETA(DisplayName = "None"),
	Player			UMETA(DisplayName = "Player"),
	Furniture		UMETA(DisplayName = "Furniture"),
	Door			UMETA(DisplayName = "Door"),
	Room			UMETA(DisplayName = "Room"),
};

/** Enum to track preference for requesting previous or next item */
UENUM(BlueprintType)
enum class EItemRotationDirection : uint8
{
	Initial			UMETA(DisplayName = "Initial Item"),
	Previous		UMETA(DisplayName = "Previous Item"),
	Next			UMETA(DisplayName = "Next Item")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPYVSSPY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UInventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	EInventoryOwnerType GetInventoryOwnerType() const { return InventoryOwnerType; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void SetInventoryOwnerType(const EInventoryOwnerType InInventoryOwnerType);

	/** Let listeners know the Equipped Weapon or Trap has changed */
	FOnEquippedUpdated OnEquippedUpdated;
	
	/**
	 * @brief Standard way to add assets to inventory as this list replicates to clients and
	 * clients then load assets from asset manager
	 * @param InPrimaryAssetIdsToLoad 
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void SetPrimaryAssetIdsToLoad(TArray<FPrimaryAssetId>& InPrimaryAssetIdsToLoad);
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	bool AddInventoryItems(TArray<FPrimaryAssetId>& PrimaryAssetIdCollectionToLoad);
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	bool RemoveInventoryItem(UInventoryItemComponent* InInventoryItem);
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void GetInventoryItems(TArray<UInventoryBaseAsset*>& InInventoryItems) const;

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void GetInventoryItemPIDs(TArray<FPrimaryAssetId>& RequestedPIDs, const FPrimaryAssetType RequestedPrimaryAssetType) const;

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	int GetCurrentCollectionSize() const { return InventoryAssetsCollection.Num(); }

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	UInventoryTrapAsset* GetRiggedTrapAsset() const { return RiggedTrapAsset; }

	/**
	 * @brief Set the owner of this inventory to have a rigged trap which activates upon interaction
	 * @param InRiggedTrapAsset The Trap Asset for rigging
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void SetRiggedTrapAsset(UInventoryTrapAsset* InRiggedTrapAsset) { RiggedTrapAsset = InRiggedTrapAsset; }


	/**
	 * Equip either a Weapon or a Trap depending on the item retrieved by InventoryIndex. Will UnEquip before Equipping
	 * @param ItemRotationDirection Whether to increment Collection Index by 1 or -1
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory|Combat")
	void EquipInventoryItem(const EItemRotationDirection ItemRotationDirection);

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|Inventory")
	EWeaponType DefaultEquippedItemType;

	UInventoryBaseAsset* GetEquippedItemAsset() const { return EquippedItemAsset; };

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory|Combat")
	void ResetEquipped();

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory|Combat")
	void EnableWeaponAttackPhase(const bool bEnableAttackPhase);

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	EInventoryOwnerType InventoryOwnerType = EInventoryOwnerType::None;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	TArray<UInventoryBaseAsset*> InventoryAssetsCollection;

	TArray<TTuple<int8, uint8, bool>> ItemsCollection;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), ReplicatedUsing = OnRep_PrimaryAssetIdsToLoad, Category = "SVS|Inventory")
	TArray<FPrimaryAssetId> PrimaryAssetIdsToLoad;
	UFUNCTION()
	void OnRep_PrimaryAssetIdsToLoad();
	
	UFUNCTION()
	void LoadInventoryAssetFromAssetId(const FPrimaryAssetId& InInventoryAssetId);

private:

	UFUNCTION(Server, Reliable)
	void S_EquipInventoryItem(const EItemRotationDirection ItemRotationDirection);

	UFUNCTION(Server, Reliable)
	void S_ResetEquipped();

	/** Weapon Index with default weapon, 255 allows for initial rep when selecting 0 */
	UPROPERTY(ReplicatedUsing = "OnRep_EquippedItemIndex")
	uint8 EquippedItemIndex = 255;
	UFUNCTION()
	void OnRep_EquippedItemIndex();
	const uint8 MaxInventorySize = 8;

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory|Combat")
	bool EquipWeapon(UInventoryWeaponAsset* WeaponAsset);

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory|Combat")
	bool EquipTrap(const UInventoryTrapAsset* TrapAsset);
	
	bool UnEquipCurrentItem();

	/** Socket Name on Character Mesh to Attach a Weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Abilities", meta = (AllowPrivateAccess = "true"))
	FName WeaponHandSocketName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Abilities", meta = (AllowPrivateAccess = "true"))
	/** Socket Name on Character Mesh to Attach a Trap */
	FName TrapHandSocketName;
	
	/** References to equipped weapon actor */
	TWeakObjectPtr<AWeapon> CurrentSpawnedWeapon;
	/** References to equipped trap mesh component */
	TWeakObjectPtr<UTrapMeshComponent> CurrentHeldTrap;

	/** Data Asset pertaining to the current Equipped Item */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	UInventoryBaseAsset* EquippedItemAsset;

	/** Asset used to determine info pertaining the rigging of a live Trap Asset */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	UInventoryTrapAsset* RiggedTrapAsset;

};
