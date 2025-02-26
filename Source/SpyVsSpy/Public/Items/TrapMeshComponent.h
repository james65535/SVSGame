// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TrapMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UTrapMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UTrapMeshComponent();

	UPROPERTY()
	FName TrapName = "";
	
};
