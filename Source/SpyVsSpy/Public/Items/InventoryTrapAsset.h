// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/InventoryBaseHeldAsset.h"
#include "GameplayTagContainer.h"
#include "InventoryTrapAsset.generated.h"

// enum class EInventoryOwnerType : uint8;

class USpyDamageEffect;
class USoundCue;
class UGameplayCueNotify_Static;
class UNiagaraSystem;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryTrapAsset : public UInventoryBaseHeldAsset
{
	GENERATED_BODY()

public:
	
	/** Animation for Character Victim Death Resulting from triggering this trap */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	UAnimMontage* CharacterDeathAnimation;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	float TrapBaseDamage = 0.0f;

	/** Used to calculate and apply damage. Needs to be set in child blueprint */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	TSubclassOf<USpyDamageEffect> TrapDamageEffectClass;
	
	/** Should Weapon apply 100% Character Max Health Damage in a Single Hit */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	bool bInstaKillEnabled = true;

	// UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	// EWeaponType WeaponType = EWeaponType::Trap;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	UGameplayCueNotify_Static* DamageGameplayCueNotify;

	/** Visual effect of the trap interacting with victim */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	UNiagaraSystem* TrapDamageVisualEffect;

	/** Visual effect of the trap interacting with victim with fatal result */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	UNiagaraSystem* TrapFatalDamageVisualEffect;

	/** Sound of the Trap interacting with victim */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	USoundCue* TrapTriggeredSoundEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap", meta = (Categories = "GameplayCue"))
	FGameplayTag GameplayTriggerTag;
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryTrapAsset", GetFName()); }
	
};
