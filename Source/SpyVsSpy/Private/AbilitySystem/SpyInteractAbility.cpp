// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyInteractAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "SVSLogger.h"
#include "AbilitySystem/AbilityTaskSuccessFailEvent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyDamageEffect.h"
#include "Items/InteractInterface.h"
#include "Items/InventoryTrapAsset.h"
#include "Players/SpyCharacter.h"
#include "Players/SpyInteractionComponent.h"
#include "Players/SpyPlayerController.h"

bool USpyInteractAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
                                             const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void USpyInteractAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if(!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{ return; }

	/** Create task to determine if interact is interrupted by a triggered trap */
		CheckTrapTriggeredTask = UAbilityTaskSuccessFailEvent::WaitSuccessFailEvent(
		this,
		FGameplayTag::RequestGameplayTag("TrapTrigger.Hit"),
		FGameplayTag::RequestGameplayTag("TrapTrigger.NoHit"),
		nullptr, true, true);
	CheckTrapTriggeredTask->SuccessEventReceived.AddDynamic(this, &ThisClass::OnTrapTriggered);
	CheckTrapTriggeredTask->FailEventReceived.AddDynamic(this, &ThisClass::OnTrapNotTriggered);

	/** Task is now listening for tags */
	CheckTrapTriggeredTask->ReadyForActivation();

	/** Trigger trap if there is one */
	const bool bTrapTriggered = RequestTriggerTrap();
	
	UE_LOG(SVSLogDebug, Warning, TEXT("GA-SpyAbilityInteract activate called for %s and trap triggered: %s"),
		*ActorInfo->AvatarActor->GetName(),
		bTrapTriggered ? *FString("True") : *FString("False"));
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USpyInteractAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AreTasksStillActive() && IsValid(CheckTrapTriggeredTask))
	{ CheckTrapTriggeredTask->EndTask(); }

	UE_LOG(SVSLogDebug, Warning, TEXT("GA-SpyAbilityInteract EndAbility and was cancelled: %s"),
		bWasCancelled ? *FString("True") : *FString("False"));
		
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USpyInteractAbility::RequestTriggerTrap()
{
	FGameplayTag TrapTriggerTaskResultTag = FGameplayTag::RequestGameplayTag("TrapTrigger.NoHit");
	
	const AActor* AbilityActor = GetActorInfo().AvatarActor.Get();

	/** Prep payload to send to gameplayevent */
	FGameplayEventData Payload = FGameplayEventData();
	Payload.Instigator = AbilityActor;
	Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(GetActorInfo().AvatarActor.Get());

	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetActorInfo().AvatarActor.Get()))
	{
		/** Determine if we can find a valid trap on the furniture and adjust Payload and Tags */
		if (IsValid(SpyCharacter->GetInteractionComponent()) &&
			SpyCharacter->GetInteractionComponent()->CanInteract())
		{
			if (const TScriptInterface<IInteractInterface> TargetInteractionComponent = SpyCharacter->
				GetInteractionComponent()->
				GetLatestInteractableComponent())
			{
				if (AActor* TargetActor = TargetInteractionComponent->
					Execute_GetInteractableOwner(TargetInteractionComponent.GetObject()))
				{
					Payload.Target = TargetActor;
					if (const UInventoryTrapAsset* TrapAsset = TargetInteractionComponent->
						Execute_GetActiveTrap(TargetInteractionComponent.GetObject()))
					{
						TrapTriggerTaskResultTag = FGameplayTag::RequestGameplayTag("TrapTrigger.Hit");
					
						/** Carry out effects for Damage */
						FActiveGameplayEffectHandle DamageGameplayEffectHandle = ApplyGameplayEffectToOwner(
							GetCurrentAbilitySpecHandle(),
							GetCurrentActorInfo(),
							GetCurrentActivationInfo(),
							SpyTrapDamageEffectClass->GetDefaultObject<UGameplayEffect>(),
							1,
							1);

						/** Carry out effects for VFX */
						FGameplayEffectContextHandle GameplayEffectContextHandle;
						GameplayEffectContextHandle.AddInstigator(SpyCharacter, TargetActor);
						
						FGameplayCueParameters GameplayCueParameters = FGameplayCueParameters(GameplayEffectContextHandle);
						GameplayCueParameters.Instigator = SpyCharacter;
						GameplayCueParameters.EffectCauser = TargetActor;

						UAbilitySystemComponent* const AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo_Checked();
						AbilitySystemComponent->ExecuteGameplayCue(TrapAsset->GameplayTriggerTag, GameplayCueParameters);
						
						/** add vfx tag to payload so it can be used later */
						Payload.TargetTags.AddTag(TrapAsset->GameplayTriggerTag);
					
						/** Remove trap from Furniture */
						TargetInteractionComponent->Execute_RemoveActiveTrap(TargetInteractionComponent.GetObject());
					}
				} 
			}
		}
	}
	
	/** Send result tag as part of a gameplay event */
	SendGameplayEvent(TrapTriggerTaskResultTag, Payload);
	
	return TrapTriggerTaskResultTag.MatchesTag(FGameplayTag::RequestGameplayTag("TrapTrigger.Hit"));
}

void USpyInteractAbility::OnTrapTriggered(FGameplayEventData Payload)
{
	/** Assumes player has died and now we clean up task */
	if (IsValid(CheckTrapTriggeredTask))
	{ CheckTrapTriggeredTask->EndTask(); }

	InteractionFail(Payload);
}

void USpyInteractAbility::OnTrapNotTriggered(FGameplayEventData Payload)
{
	if (CanInteract())
	{
		if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetActorInfo().AvatarActor.Get()))
		{
			const UObject* InteractableComponent = SpyCharacter->
				GetInteractionComponent()->
				GetLatestInteractableComponent().GetObject();
			
			if (IsValid(InteractableComponent))
			{ Payload.OptionalObject = InteractableComponent; }

			InteractionSuccess(Payload);
		}
	}
	else
	{ InteractionFail(Payload); }
}

bool USpyInteractAbility::CanInteract() const
{
	if (const ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetActorInfo().AvatarActor.Get()))
	{
		if (IsValid(Cast<UInventoryTrapAsset>(SpyCharacter->GetHeldWeapon())))
		{
			if (const ASpyPlayerController* PlayerController = Cast<ASpyPlayerController>(SpyCharacter->GetController()))
			{
				/**
				 * Spy places trap but is not allowed to interact further
				 * Request Place trap returns true if trap was placed
				 * We invert the return to indicate we cannot interact further
				 */
				return !PlayerController->RequestPlaceTrap();
			}
		}
	}
	// TODO could refactor this in future if not casting to spy character impacts decision
	return true;
}

void USpyInteractAbility::InteractionSuccess_Implementation(FGameplayEventData Payload)
{
	UE_LOG(SVSLogDebug, Warning, TEXT("GA-SpyAbilityInteract interaction success called"));
}

void USpyInteractAbility::InteractionFail_Implementation(FGameplayEventData Payload)
{
	UE_LOG(SVSLogDebug, Warning, TEXT("GA-SpyAbilityInteract interaction failed called"));
}
