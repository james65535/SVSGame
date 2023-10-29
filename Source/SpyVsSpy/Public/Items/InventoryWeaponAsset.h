// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/WeaponComponent.h"
#include "Items/InventoryBaseAsset.h"
#include "InventoryWeaponAsset.generated.h"

//enum class EWeaponType : uint8;
class USoundCue;
class UNiagaraSystem;
class UWeaponComponent;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryWeaponAsset : public UInventoryBaseAsset
{
	GENERATED_BODY()

public:

	/** Animation for Attacking Character with this Weapon */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	UAnimationAsset* CharacterAttackAnimation;

	/** Animation for Character Victim Death Resulting from this weapon inflicting the final blow */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	UAnimationAsset* CharacterDeathAnimation;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	float WeaponAttackBaseDamage = 0.0f;

	/** Should Weapon apply 100% Character Max Health Damage in a Single Hit */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	bool bInstaKillEnabled = false;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	EWeaponType WeaponType = EWeaponType::None;
	//TSubclassOf<UWeaponComponent> WeaponType;

	/** Visual effect for the action of using the weapon */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	UNiagaraSystem* AttackVisualEffect;

	/** Visual effect of the weapon interacting with victim */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	UNiagaraSystem* AttackDamageVisualEffect;

	/** Visual effect of the weapon interacting with victim with fatal result */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	UNiagaraSystem* AttackFatalDamageVisualEffect;

	/** Sound for the action of using the weapon */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	USoundCue* AttackSoundEffect;

	/** Sound of the weapon interacting with victim */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	USoundCue* AttackDamageSoundEffect;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryWeaponAsset", GetFName()); }
	
};
