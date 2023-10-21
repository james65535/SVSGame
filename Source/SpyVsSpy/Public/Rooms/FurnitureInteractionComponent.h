// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/InteractionComponent.h"
#include "FurnitureInteractionComponent.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UFurnitureInteractionComponent : public UInteractionComponent
{
	GENERATED_BODY()

public:

	/** Interact Interface Override */
	/** @return Success Status */
	virtual bool Interact_Implementation(AActor* InteractRequester) override;
};
