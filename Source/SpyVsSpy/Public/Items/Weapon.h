// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryWeaponAsset.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class USkeletalMeshComponent;

UCLASS()
class SPYVSSPY_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();
	// TODO refactor into base virtual class
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Loads property values from the data asset assigned to the instance of this class
	* @return Whether Loading was Successful or Not */
	UFUNCTION()
	bool LoadWeaponPropertyValuesFromDataAsset(const UInventoryWeaponAsset* InventoryWeaponAsset);

	UFUNCTION(BlueprintCallable, Category = "SVS|Weapon")
	USkeletalMeshComponent* GetMesh() const { return SkeletalMeshComponent; }
	
	/** Enable Components sweeps to be performed on tick */
	void EnableOnTickComponentSweeps(const bool bEnable);

	UFUNCTION()
	void UpdateCollisionChannelResponseToBlock(const ECollisionChannel EnemyObjectChannel, const ECollisionChannel SelfObjectChannel);


private:
	
	virtual void BeginPlay() override;

	/** Whether we perform Component Sweeps on Tick */
	UPROPERTY(ReplicatedUsing="OnRep_bEnableOnTickComponentSweeps")
	bool bEnableOnTickComponentSweeps = false;
	void SweepWeapon();

	/** This allows clients to perform ComponentSweep as well for cosmetic effects */
	UFUNCTION()
	void OnRep_bEnableOnTickComponentSweeps();

	FVector OnComponentSweepEnableStartLocation = FVector::ZeroVector;
	FComponentQueryParams SweepQueryParams = FComponentQueryParams::DefaultComponentQueryParams;

	UPROPERTY(ReplicatedUsing="OnRep_SetMesh")
	USkeletalMesh* WeaponMesh;
	
	UFUNCTION()
	void OnRep_SetMesh();

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS|Weapon")
	USkeletalMeshComponent* SkeletalMeshComponent;
	
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS|Weapon")
	APawn* PawnOwner;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Inventory|Combat")
	EWeaponType WeaponType = EWeaponType::None;
	
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS|Weapon")
	float WeaponAttackBaseDamage = 0.0f;

	/** Animation for Attacking Character with this Weapon */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	UAnimationAsset* WeaponAttackAnimation;

	/** Should Weapon apply 100% Character Max Health Damage in a Single Hit */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	bool bInstaKillEnabled = false;

	/** Visual effect for the action of using the weapon */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	UNiagaraSystem* AttackVisualEffect;

	/** Visual effect of the weapon interacting with victim */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	UNiagaraSystem* AttackDamageVisualEffect;

	/** Visual effect of the weapon interacting with victim with fatal result */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	UNiagaraSystem* AttackFatalDamageVisualEffect;

	/** Sound for the action of using the weapon */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	USoundCue* AttackSoundEffect;

	/** Sound of the weapon interacting with victim */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	USoundCue* AttackDamageSoundEffect;

	/** Weapon Fire Impact Related Date */
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	FVector ImpactPositions;
	UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	FVector ImpactNormals;
	// UPROPERTY(BlueprintReadOnly, EditInstanceOnly,Meta = (AllowPrivateAccess = "true"), Category = "SVS|Weapon")
	// TEnumAsByte<enum EPhysicalSurface> PhysicalSurfaceType;


};
