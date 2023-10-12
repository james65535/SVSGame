// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTaskSuccessFailEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UAbilityTaskSuccessFailEvent* UAbilityTaskSuccessFailEvent::WaitSuccessFailEvent(UGameplayAbility* OwningAbility, const FGameplayTag SuccessTag, const FGameplayTag FailTag, AActor* OptionalExternalTarget, const bool OnlyTriggerOnce,  const bool OnlyMatchExact)
{
	UAbilityTaskSuccessFailEvent* MyObj = NewAbilityTask<UAbilityTaskSuccessFailEvent>(OwningAbility);
	MyObj->SuccessTag = SuccessTag;
	MyObj->FailTag = FailTag;
	MyObj->SetExternalTarget(OptionalExternalTarget);
	MyObj->OnlyTriggerOnce = OnlyTriggerOnce;
	MyObj->OnlyMatchExact = OnlyMatchExact;

	return MyObj;
}

void UAbilityTaskSuccessFailEvent::SetExternalTarget(const AActor* Actor)
{
	if (Actor)
	{
		UseExternalTarget = true;
		OptionalExternalTarget = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	}
}

UAbilitySystemComponent* UAbilityTaskSuccessFailEvent::GetTargetAbilitySystemComponent() const
{
	if (UseExternalTarget) { return OptionalExternalTarget; }

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
	
	if (OnlyTriggerOnce) { EndTask(); }
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
	
	if (OnlyTriggerOnce) { EndTask(); }
}

void UAbilityTaskSuccessFailEvent::OnDestroy(bool AbilityEnding)
{
	UAbilitySystemComponent* ASC = GetTargetAbilitySystemComponent();
	if (ASC && SuccessHandle.IsValid())
	{
		if (OnlyMatchExact)
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(SuccessTag).Remove(SuccessHandle);
		}
		else
		{
			ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(SuccessTag), SuccessHandle);
		}
	}

	if (ASC && FailHandle.IsValid())
	{
		if (OnlyMatchExact)
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(FailTag).Remove(FailHandle);
		}
		else
		{
			ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(FailTag), FailHandle);
		}
	}
	
	Super::OnDestroy(AbilityEnding);
}
