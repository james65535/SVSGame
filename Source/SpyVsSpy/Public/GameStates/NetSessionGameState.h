// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "NetSessionGameState.generated.h"

class ARoomManager;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API ANetSessionGameState : public AGameState
{
	GENERATED_BODY()

public:
	ANetSessionGameState();
	
	ARoomManager* GetRoomManager() const;
	
	/** Class Overrides */
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess), ReplicatedUsing="OnRep_RoomManager", Category = "SVS Room")
	ARoomManager* RoomManager;

	UFUNCTION()
	void OnRep_RoomManager();
	
};
