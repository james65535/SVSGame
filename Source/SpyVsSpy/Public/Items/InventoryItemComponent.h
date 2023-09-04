// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
//#include "Blueprint/UserWidget.h"
#include "InventoryItemComponent.generated.h"

class ASpyCharacter;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemPickup);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemUsed);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Category = "SVS Inventory Item" )
class SPYVSSPY_API UInventoryItemComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryItemComponent();

	/** Delegates */
	FOnItemPickup OnItemPickup;
	FOnItemUsed OnItemUsed;
	
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	FName GetItemName() const { return ItemName; }
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	FString GetItemDescription() const { return ItemDescription; }
	// TODO Setup Widget for Item Image
	// UFUNCTION(BlueprintCallable, Category = "Inventory Item")
	// TSubclassOf<UUserWidget> GetItemPicture() const;
	UFUNCTION(BlueprintCallable, Category = "Inventory Item")
	bool GetIsItemValid() const { return bIsItemValid; }
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	void SetItemIsValid(bool const bIsValid = false) { bIsItemValid = bIsValid; }
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	TSoftObjectPtr<AActor> GetPossessingActor() { return PossessingActor; }
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	void PlayUseSound() const ;

	/** @return success status */
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	bool PickUpItem(ASpyCharacter* InCollectingActor);
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	void UseItem();

private:

	/** Item Properties */
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS Inventory Item")
	FName ItemName = "Null Item Name";
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS Inventory Item")
	FString ItemDescription = "Null Item Description";
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS Inventory Item")
	UStaticMeshComponent* Mesh;
	// TODO Setup Widget for Item Image
	// UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "Inventory Item")
	// TSubclassOf<UUserWidget> ItemImage;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS Inventory Item")
	TSoftObjectPtr<AActor> PossessingActor;
	bool bIsItemValid = false;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS Inventory Item")
	USoundBase* UseSound;

};
