// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryComponent.h"
#include "Items/InventoryWeaponAsset.h"
#include "InventoryTrapAsset.generated.h"

enum class EInventoryOwnerType : uint8;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryTrapAsset : public UInventoryBaseAsset
{
	GENERATED_BODY()

public:

	/** Type of inventory owner this trap may be used on */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	EInventoryOwnerType InventoryOwnerType = EInventoryOwnerType::None;

	/** Local Space Transform for Effect */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	FTransform TrapEffectTransformOffset;

	/** Static Mesh for Trap */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (AllowPrivateAccess), Category = "SVS|Inventory|Trap")
	UStaticMesh* TrapMesh;
	
	/** Attach Transform for Weapon Actor */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	FTransform HeldTrapAttachTransform;

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

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	EWeaponType WeaponType = EWeaponType::Trap;

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
