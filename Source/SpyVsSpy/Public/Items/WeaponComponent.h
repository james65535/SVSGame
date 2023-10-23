// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/InventoryItemComponent.h"
#include "WeaponComponent.generated.h"

class UNiagaraSystem;
class USoundCue;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None UMETA(DisplayName = "None"),
	Trap UMETA(DisplayName = "Trap"),
	Gun	UMETA(DisplayName = "Gun"),
	Knife UMETA(DisplayName = "Knife"),
	Club UMETA(DisplayName = "Club"),
};

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UWeaponComponent : public UInventoryItemComponent
{
	GENERATED_BODY()

public:

	UWeaponComponent();

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	FPrimaryAssetId WeaponAssetData;
	
#pragma region="Getter/Setters"
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory|Combat")
	float GetBaseDamage() const { return WeaponAttackBaseDamage; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Inventory|Combat")
	EWeaponType GetWeaponType() const { return WeaponType; }
#pragma endregion="Getter/Setters"

protected:

	/** Loads property values from the data asset assigned to the instance of this class
	 * @return Whether Loading was Successful or Not */
	UFUNCTION()
	bool LoadWeaponPropertyValuesFromDataAsset();
	
private:

#pragma region="ItemProperties"
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	EWeaponType WeaponType = EWeaponType::None;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS|Inventory|Combat")
	float WeaponAttackBaseDamage = 0.0f;

	/** Animation for Attacking Character with this Weapon */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	UAnimationAsset* CharacterAttackAnimation;

	/** Animation for Character Victim Death Resulting from this weapon inflicting the final blow */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	UAnimationAsset* CharacterDeathAnimation;

	/** Should Weapon apply 100% Character Max Health Damage in a Single Hit */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	bool bInstaKillEnabled = false;

	/** Visual effect for the action of using the weapon */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	UNiagaraSystem* AttackVisualEffect;

	/** Visual effect of the weapon interacting with victim */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	UNiagaraSystem* AttackDamageVisualEffect;

	/** Visual effect of the weapon interacting with victim with fatal result */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	UNiagaraSystem* AttackFatalDamageVisualEffect;

	/** Sound for the action of using the weapon */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	USoundCue* AttackSoundEffect;

	/** Sound of the weapon interacting with victim */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	USoundCue* AttackDamageSoundEffect;
#pragma endregion="ItemProperties"
};
