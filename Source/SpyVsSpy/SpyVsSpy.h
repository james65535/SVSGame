// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define COLLISION_INTERACT			ECollisionChannel::ECC_GameTraceChannel1
#define PROJECT_PATH				"/Script/SpyVsSpy"
#define ABILITY_INPUT_ID			"ESVSAbilityInputID"


UENUM(BlueprintType)
enum class ESVSAbilityInputID : uint8
{
	// 0 None
	None					UMETA(DisplayName = "None"),
	// 1 Confirm
	Confirm					UMETA(DisplayName = "Confirm"),
	// 2 Cancel
	Cancel					UMETA(DisplayName = "Cancel"),
	// 3 Sprint
	SprintAction			UMETA(DisplayName = "Sprint"),
	// 4 Jump
	JumpAction				UMETA(DisplayName = "Jump"),
	// 5 PrimaryAttack
	PrimaryAttackAction		UMETA(DisplayName = "Primary Attack"),
	// 6 NextTrap
	NextTrapAction			UMETA(DisplayName = "Next Trap"), 
	// 7 PrevTrap
	PrevTrapAction			UMETA(DisplayName = "Previous Trap"),
	// 8 Interact
	InteractAction			UMETA(DisplayName = "Interact"),
	// 9 Interact
	TrapTriggerAction		UMETA(DisplayName = "TrapTrigger")
};
