// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UInventoryTrapAsset;
class UInventoryItemComponent;
class UInventoryBaseAsset;

DECLARE_MULTICAST_DELEGATE(FOnInventoryUpdated);

UENUM(BlueprintType)
enum class EInventoryOwnerType : uint8
{
	None			UMETA(DisplayName = "None"),
	Player			UMETA(DisplayName = "Player"),
	Furniture		UMETA(DisplayName = "Furniture"),
	Door			UMETA(DisplayName = "Door"),
	Room			UMETA(DisplayName = "Room"),
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
	
	/**
	 * @brief Standard way to add assets to inventory as this list replicates to clients and
	 * clients then load assets from asset manager
	 * @param InPrimaryAssetIdsToLoad 
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void SetPrimaryAssetIdsToLoad(TArray<FPrimaryAssetId>& InPrimaryAssetIdsToLoad);
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	bool AddInventoryItems(TArray<FPrimaryAssetId>& PrimaryAssetIdCollectionToLoad);
	//UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	//bool AddInventoryItemFromPrimaryAssetID(FPrima& InventoryItemAssets);
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	bool RemoveInventoryItem(UInventoryItemComponent* InInventoryItem);
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void GetInventoryItems(TArray<UInventoryBaseAsset*>& InInventoryItems) const;

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	void GetInventoryItemPIDs(TArray<FPrimaryAssetId>& RequestedPIDs, const FPrimaryAssetType RequestedPrimaryAssetType) const;

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	int GetCurrentCollectionSize() const { return InventoryCollection.Num(); }

	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	UInventoryTrapAsset* GetActiveTrap() const { return ActiveTrap; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory")
	bool SetActiveTrap(UInventoryTrapAsset* InActiveTrap);
	
	//FOnInventoryUpdated OnInventoryUpdated;

protected:

	const uint8 MaxInventorySize = 8;

	// UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), ReplicatedUsing = OnRep_InventoryCollection, Category = "SVS|Inventory")
	// TArray<UInventoryBaseAsset*> InventoryCollection;
	// UFUNCTION()
	// void OnRep_InventoryCollection() const;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	EInventoryOwnerType InventoryOwnerType = EInventoryOwnerType::None;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	TArray<UInventoryBaseAsset*> InventoryCollection;

	TArray<TTuple<int8, uint8, bool>> ItemsCollection;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess), ReplicatedUsing = OnRep_PrimaryAssetIdsToLoad, Category = "SVS|Inventory")
	TArray<FPrimaryAssetId> PrimaryAssetIdsToLoad;
	UFUNCTION()
	void OnRep_PrimaryAssetIdsToLoad();
	
	UFUNCTION()
	void LoadInventoryAssetFromAssetId(const FPrimaryAssetId& InInventoryAssetId);
	// UFUNCTION()
	// void OnInventoryAssetLoad(const FPrimaryAssetId InInventoryAssetId);

	// TODO secure UPROP settings
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	UInventoryTrapAsset* ActiveTrap;
	// UFUNCTION()
	// void OnRep_ActiveTrap();
		
};
