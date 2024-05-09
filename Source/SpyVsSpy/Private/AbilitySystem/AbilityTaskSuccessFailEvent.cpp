// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTaskSuccessFailEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SVSLogger.h"

UAbilityTaskSuccessFailEvent* UAbilityTaskSuccessFailEvent::WaitSuccessFailEvent(UGameplayAbility* OwningAbility, const FGameplayTag SuccessTag, const FGameplayTag FailTag, AActor* OptionalExternalTarget, const bool OnlyTriggerOnce,  const bool OnlyMatchExact)
{
	UAbilityTaskSuccessFailEvent* AbilityTaskSuccessFailEvent = NewAbilityTask<UAbilityTaskSuccessFailEvent>(OwningAbility);
	AbilityTaskSuccessFailEvent->SuccessTag = SuccessTag;
	AbilityTaskSuccessFailEvent->FailTag = FailTag;
	AbilityTaskSuccessFailEvent->SetExternalTarget(OptionalExternalTarget);
	AbilityTaskSuccessFailEvent->OnlyTriggerOnce = OnlyTriggerOnce;
	AbilityTaskSuccessFailEvent->OnlyMatchExact = OnlyMatchExact;

	return AbilityTaskSuccessFailEvent;
}

void UAbilityTaskSuccessFailEvent::SetExternalTarget(const AActor* Actor)
{
	if (IsValid(Actor))
	{
		UseExternalTarget = true;
		OptionalExternalTarget = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	}
}

UAbilitySystemComponent* UAbilityTaskSuccessFailEvent::GetTargetAbilitySystemComponent() const
{
	if (UseExternalTarget)
	{ return OptionalExternalTarget; }

	return AbilitySystemComponent.Get();
}

void UAbilityTaskSuccessFailEvent::Activate()
{
	if (UAbilitySystemComponent* TargetAbilitySystemComponent = GetTargetAbilitySystemComponent())
	{
		if (OnlyMatchExact)
		{
			SuccessHandle = TargetAbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(SuccessTag).AddUObject(
				this,
				&UAbilityTaskSuccessFailEvent::SuccessEventCallback);

			FailHandle = TargetAbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(FailTag).AddUObject(
				this,
				&UAbilityTaskSuccessFailEvent::FailEventCallback);
		}
		else
		{
			SuccessHandle = TargetAbilitySystemComponent->AddGameplayEventTagContainerDelegate(
				FGameplayTagContainer(SuccessTag),
				FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(
					this,
					&UAbilityTaskSuccessFailEvent::SuccessEventContainerCallback));

			FailHandle = TargetAbilitySystemComponent->AddGameplayEventTagContainerDelegate(
				FGameplayTagContainer(FailTag),
				FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(
					this,
					&UAbilityTaskSuccessFailEvent::FailEventContainerCallback));
		}	
	}
	Super::Activate();
}

void UAbilityTaskSuccessFailEvent::SuccessEventCallback(const FGameplayEventData* Payload)
{
	SuccessEventContainerCallback(SuccessTag, Payload);
}

void UAbilityTaskSuccessFailEvent::SuccessEventContainerCallback(FGameplayTag MatchingTag,
	const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempPayload = *Payload;
		TempPayload.EventTag = MatchingTag;
		SuccessEventReceived.Broadcast(MoveTemp(TempPayload));
	}
	
	if (OnlyTriggerOnce)
	{ EndTask(); }
}

void UAbilityTaskSuccessFailEvent::FailEventCallback(const FGameplayEventData* Payload)
{
	FailEventContainerCallback(FailTag, Payload);
}

void UAbilityTaskSuccessFailEvent::FailEventContainerCallback(FGameplayTag MatchingTag,
	const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempPayload = *Payload;
		TempPayload.EventTag = MatchingTag;
		FailEventReceived.Broadcast(MoveTemp(TempPayload));
	}
	
	if (OnlyTriggerOnce)
	{ EndTask(); }
}

void UAbilityTaskSuccessFailEvent::OnDestroy(bool AbilityEnding)
{
	UAbilitySystemComponent* TargetAbilitySystemComponent = GetTargetAbilitySystemComponent();
	if (TargetAbilitySystemComponent && SuccessHandle.IsValid())
	{
		if (OnlyMatchExact)
		{ TargetAbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(SuccessTag).Remove(SuccessHandle); }
		else
		{ TargetAbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(SuccessTag), SuccessHandle); }
	}

	if (TargetAbilitySystemComponent && FailHandle.IsValid())
	{
		if (OnlyMatchExact)
		{ TargetAbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(FailTag).Remove(FailHandle); }
		else
		{ TargetAbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(FailTag), FailHandle); }
	}
	
	
	Super::OnDestroy(AbilityEnding);
}
