// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InteractionComponent.h"

#include "SVSLogger.h"

UInteractionComponent::UInteractionComponent()
{

}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UInteractionComponent::SetInteractionEnabled(const bool bIsEnabled)
{
	bInteractionEnabled = bIsEnabled;
}

bool UInteractionComponent::IsInteractionEnabled() const
{
	return bInteractionEnabled;
}

bool UInteractionComponent::Interact_Implementation(AActor* InteractRequester)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Actor: %s was requested to interact by %s"), *GetOwner()->GetName(), *InteractRequester->GetName());
	return IsInteractionEnabled();
}

void UInteractionComponent::ProvideInventoryListing_Implementation(TArray<UInventoryItemComponent*>& InventoryItems)
{
	IInteractInterface::ProvideInventoryListing_Implementation(InventoryItems);
}
