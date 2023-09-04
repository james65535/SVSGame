// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/InventoryItemComponent.h"
#include "WeaponComponent.generated.h"


UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Gun	UMETA(DisplayName = "Gun"),
	Knife UMETA(DisplayName = "Knife"),
	Club UMETA(DisplayName = "Club")
};


/**
 * 
 */
UCLASS()
class SPYVSSPY_API UWeaponComponent : public UInventoryItemComponent
{
	GENERATED_BODY()

public:
	
	/** Item Getter / Setters */
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	float GetBaseDamage() const {return BaseDamage; }
	UFUNCTION(BlueprintCallable, Category = "SVS Inventory Item")
	EWeaponType GetWeaponType() const { return WeaponType; }
	
private:

	/** Item Properties */
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS Inventory Item")
	EWeaponType WeaponType;
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS Inventory Item")
	float BaseDamage;
};
