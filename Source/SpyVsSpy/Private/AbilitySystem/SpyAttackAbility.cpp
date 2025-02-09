// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyAttackAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "SVSLogger.h"
#include "AbilitySystem/AbilityTaskSuccessFailEvent.h"
#include "AbilitySystem/SpyAbilityCoolDownEffect.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyDamageEffect.h"
#include "Items/InventoryWeaponAsset.h"
#include "Players/SpyCharacter.h"

USpyAttackAbility::USpyAttackAbility()
{
	CheckAttackResultTask = nullptr;
	bRetriggerInstancedAbility = false;
	AbilityInputID = ESpyAbilityInputID::PrimaryAttackAction;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	bServerRespectsRemoteAbilityCancellation = false;
	AttackHitTag = FGameplayTag::RequestGameplayTag("Attack.Hit");
	AttackMissTag = FGameplayTag::RequestGameplayTag("Attack.NoHit");
	CooldownGameplayEffectClass = USpyAbilityCoolDownEffect::StaticClass();
}

bool USpyAttackAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
                                           const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void USpyAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if(!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(SVSLog, Warning, TEXT("GA-SpyAttack could not commit ability"));
		CancelAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true);
		return;
	}
	
	/** Create task to determine if attack was successful */
	CheckAttackResultTask = UAbilityTaskSuccessFailEvent::WaitSuccessFailEvent(
	this,
	AttackHitTag,
	AttackMissTag,
	nullptr, true, true);
	CheckAttackResultTask->SuccessEventReceived.AddDynamic(this, &ThisClass::OnAttackHit);
	CheckAttackResultTask->FailEventReceived.AddDynamic(this, &ThisClass::OnAttackMiss);

	/** Task is now listening for tags */
	CheckAttackResultTask->ReadyForActivation();
	/** Trigger trap if there is one */
	if (CheckAttackResultTask->IsActive())
	{
		const bool bAttackRequestSuccess = RequestAttack();
		if (bAttackRequestSuccess)
		{
			Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
			return;
		}
	}
	
	CancelAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true);
}

bool USpyAttackAbility::RequestAttack() const
{
	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetCurrentActorInfo()->AvatarActor.Get()))
	{
		const UInventoryWeaponAsset* WeaponAsset = Cast<UInventoryWeaponAsset>(SpyCharacter->GetEquippedItemAsset());
		if (IsValid(WeaponAsset) && IsValid(WeaponAsset->CharacterAttackAnimation))
		{
			SpyCharacter->PlayAttackAnimation(WeaponAsset->CharacterAttackAnimation);
			return true;
		}
	} 
	return false;
}

void USpyAttackAbility::OnAttackHit(FGameplayEventData Payload)
{
	if (ASpyCharacter* AttackingSpyCharacter = Cast<ASpyCharacter>(Payload.Instigator))
	{
		const UInventoryWeaponAsset* WeaponAsset = Cast<UInventoryWeaponAsset>(AttackingSpyCharacter->GetEquippedItemAsset());
		if (IsValid(WeaponAsset) &&
			WeaponAsset->GameplayTriggerTag.IsValid() &&
			IsValid(WeaponAsset->SpyAttackDamageEffectClass))
		{
			/** Apply Damage */
			TArray<FActiveGameplayEffectHandle> ActiveDamageEffectSpecHandles = ApplyGameplayEffectToTarget(
				GetCurrentAbilitySpecHandle(),
				GetCurrentActorInfo(),
				GetCurrentActivationInfo(),
				Payload.TargetData,
				WeaponAsset->SpyAttackDamageEffectClass,
				1,
				1);
		
			/** Perform VFX */
			FGameplayCueParameters GameplayCueParameters = FGameplayCueParameters(Payload.ContextHandle);
			GameplayCueParameters.Instigator = AttackingSpyCharacter;
			GameplayCueParameters.EffectCauser = AttackingSpyCharacter;
			GameplayCueParameters.SourceObject = WeaponAsset;
			GameplayCueParameters.Location = Payload.TargetData.Get(0)->GetHitResult()->ImpactPoint;
			
			/** add vfx tag to payload so it can be used later */
			// Payload.TargetTags.AddTag(WeaponAsset->GameplayTriggerTag);
			UAbilitySystemComponent* const AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo_Checked();
			AbilitySystemComponent->ExecuteGameplayCue(WeaponAsset->GameplayTriggerTag, GameplayCueParameters);
			
			EndAbility(
				GetCurrentAbilitySpecHandle(),
				GetCurrentActorInfo(),
				GetCurrentActivationInfo(),
				true,
				false);
			return;
		}
		UE_LOG(SVSLog, Warning, TEXT("GA-SpyAttack on attack callback has ran but weaponasset or triggertag is not valid"));
	}
	CancelAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true);
}

void USpyAttackAbility::OnAttackMiss(FGameplayEventData Payload)
{
	CancelAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true);
}

void USpyAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	/** Clean up Result Task if it is still running */
	if (AreTasksStillActive() && IsValid(CheckAttackResultTask))
	{ CheckAttackResultTask->EndTask(); }
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
