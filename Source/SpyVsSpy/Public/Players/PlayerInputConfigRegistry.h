// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnhancedInput/Public/InputAction.h"
#include "PlayerInputConfigRegistry.generated.h"

/**
 * Enhanced Input Action Registry
 */

UCLASS()
class SPYVSSPY_API UPlayerInputConfigRegistry : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputLook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputJump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputCrouch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputSprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputInteract;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* PrimaryAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* NextTrapAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* PrevTrapAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputOpenMenuAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputCloseMenuAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* OpenTargetInventoryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* CloseInventoryAction;
};
