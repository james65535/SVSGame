// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "UHeldItemMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UHeldItemMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UHeldItemMeshComponent();
	virtual void BeginPlay() override;
	
	UPROPERTY()
	FName HeldItemName = "";
	
};
