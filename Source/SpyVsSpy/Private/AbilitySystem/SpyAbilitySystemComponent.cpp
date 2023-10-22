// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyAbilitySystemComponent.h"

USpyAbilitySystemComponent::USpyAbilitySystemComponent()
{
	bCharacterAbilitiesGiven  = false;
}

void USpyAbilitySystemComponent::ReceiveDamage(const USpyAbilitySystemComponent* SourceAbilitySystemComponent, float DamageDone)
{
	ReceivedDamageDelegate.Broadcast(SourceAbilitySystemComponent, DamageDone);
}

void USpyAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	// if (!bAbilitySystemInitiated)
	// {
		Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
		bAbilitySystemInitiated = true;
	//}
}
