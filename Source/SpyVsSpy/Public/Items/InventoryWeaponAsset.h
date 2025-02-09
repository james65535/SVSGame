// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/SpyDamageEffect.h"
#include "Items/InventoryBaseAsset.h"
#include "InventoryWeaponAsset.generated.h"

class AWeapon;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None UMETA(DisplayName = "None"),
	Trap UMETA(DisplayName = "Trap"),
	Gun	UMETA(DisplayName = "Gun"),
	Knife UMETA(DisplayName = "Knife"),
	Club UMETA(DisplayName = "Club"),
};

class USoundCue;
class UNiagaraSystem;
class UWeaponComponent;
class UGameplayCueNotify_Static;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryWeaponAsset : public UInventoryBaseAsset
{
	GENERATED_BODY()

public:

	/** Class for Weapon Actor */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	TSubclassOf<AWeapon> WeaponClass;

	/** Skeletal Mesh for Weapon Actor */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	USkeletalMesh* WeaponMesh;

	/** Attach Transform for Weapon Actor */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	FTransform WeaponAttachTransform;

	/** Animation for Attacking Character with this Weapon */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	UAnimMontage* CharacterAttackAnimation;

	/** Animation for Character Victim Death Resulting from this weapon inflicting the final blow */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	UAnimMontage* CharacterDeathAnimation;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	float WeaponAttackBaseDamage = 0.0f;

	/** Used to calculate and apply damage. Needs to be set in child blueprint */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	TSubclassOf<USpyDamageEffect> SpyAttackDamageEffectClass;
	
	/** Should Weapon apply 100% Character Max Health Damage in a Single Hit */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	bool bInstaKillEnabled = false;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	EWeaponType WeaponType = EWeaponType::None;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat")
	UGameplayCueNotify_Static* DamageGameplayCueNotify;

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

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Combat", meta = (Categories = "GameplayCue" ))
	FGameplayTag GameplayTriggerTag;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryWeaponAsset", GetFName()); }
	
};
