// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponComponent.h"

#include "Engine/AssetManager.h"

UWeaponComponent::UWeaponComponent()
{
	// Load data asset and test valid
	/** Load Weapon Data Asset and Test Validity */
	if (WeaponAssetData.IsValid())
	{
		//UAssetManager.load

		LoadWeaponPropertyValuesFromDataAsset();
	}

}

bool UWeaponComponent::LoadWeaponPropertyValuesFromDataAsset()
{
	// if (!IsValid())
	// { return false; }
	
	WeaponType = EWeaponType::None;

	/** Damage Info */
	bInstaKillEnabled = false;
	if (!bInstaKillEnabled)
	{ WeaponAttackBaseDamage = 0.0f; }

	/** Associated Animations */
	CharacterAttackAnimation = nullptr;
	CharacterDeathAnimation = nullptr;

	/** VFX */
	AttackVisualEffect = nullptr;
	AttackDamageVisualEffect = nullptr;
	AttackFatalDamageVisualEffect = nullptr;

	/** SFX */
	AttackSoundEffect = nullptr;
	AttackDamageSoundEffect = nullptr;

	return true;
}
