// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventoryItemComponent.h"
#include "Players/SpyCharacter.h"
//#include "Blueprint/UserWidget.h"

// Sets default values for this component's properties
UInventoryItemComponent::UInventoryItemComponent()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh Component");
	if (IsValid(Mesh) && IsRooted())
	{
		Mesh->SetupAttachment(this);
	}
	UseSound = CreateDefaultSubobject<USoundBase>("Sound Component");
}

// TSubclassOf<UUserWidget> UInventoryItemComponent::GetItemPicture() const
// {
// 	// TODO 
// 	return ItemImage;
// }

void UInventoryItemComponent::PlayUseSound() const
{
	if (IsValid(UseSound))
	{
		// TODO
	}
}

bool UInventoryItemComponent::PickUpItem(ASpyCharacter* InCollectingActor)
{

	if (!IsValid(InCollectingActor)){ return false; }

	
	// TODO Perform Validity Checks
	
	OnItemPickup.Broadcast();
	return  false;
	
}

void UInventoryItemComponent::UseItem()
{
	// TODO

	PlayUseSound();
	OnItemUsed.Broadcast();
}

