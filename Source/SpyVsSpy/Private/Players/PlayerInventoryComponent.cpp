// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/PlayerInventoryComponent.h"
#include "Items/InventoryItemComponent.h"

// Sets default values for this component's properties
UPlayerInventoryComponent::UPlayerInventoryComponent()
{

}

bool UPlayerInventoryComponent::AddInventoryItem(UInventoryItemComponent* InInventoryItem)
{
	if (!IsValid(InInventoryItem) || InventoryCollection.Num() == MaxInventorySize) { return false; }
	
	if (InventoryCollection.Emplace(InInventoryItem) >= 0) { return true; }
	
	return false;
}

bool UPlayerInventoryComponent::RemoveInventoryItem(UInventoryItemComponent* InInventoryItem)
{
	if (!IsValid(InInventoryItem)) { return false; }
	
	// TODO validation checks

	if(InventoryCollection.RemoveSingle(InInventoryItem) == 1)
	{
		InInventoryItem->DestroyComponent();
		return true;
	}
	return false;
}

void UPlayerInventoryComponent::GetInventoryItems(TArray<UInventoryItemComponent*>& InventoryItems) const
{

	// TODO

	if (!InventoryCollection.IsEmpty())
	{
		InventoryItems = InventoryCollection;
	}
}
