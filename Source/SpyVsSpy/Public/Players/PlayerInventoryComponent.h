// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerInventoryComponent.generated.h"


class UInventoryItemComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPYVSSPY_API UPlayerInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "SVS Character")
	bool AddInventoryItem(UInventoryItemComponent* InInventoryItem);
	UFUNCTION(BlueprintCallable, Category = "SVS Character")
	bool RemoveInventoryItem(UInventoryItemComponent* InInventoryItem);
	UFUNCTION(BlueprintCallable, Category = "SVS Character")
	void GetInventoryItems(TArray<UInventoryItemComponent*>& InventoryItems) const;

private:

	const uint8 MaxInventorySize = 8;
	UPROPERTY()
	TArray<UInventoryItemComponent*> InventoryCollection;
		
};
