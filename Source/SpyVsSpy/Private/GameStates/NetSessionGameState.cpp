// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStates/NetSessionGameState.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "GameModes/NetworkSessionGameMode.h"
#include "Rooms/RoomManager.h"

ANetSessionGameState::ANetSessionGameState()
{
	bReplicates = true;
}

void ANetSessionGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;
	SharedParams.Condition = COND_SkipOwner;

	DOREPLIFETIME_WITH_PARAMS_FAST(ANetSessionGameState, RoomManager, SharedParams);
}

void ANetSessionGameState::BeginPlay()
{
	/** Load RoomManager and persist reference for replication to clients */
	if (!IsValid(RoomManager))
	{
		if(ANetworkSessionGameMode* GameMode = Cast<ANetworkSessionGameMode>(AuthorityGameMode))
		{
			RoomManager = GameMode->LoadRoomManager();
			MARK_PROPERTY_DIRTY_FROM_NAME(ANetSessionGameState, RoomManager, this);
		}
		if (!IsValid(RoomManager))
		{
			UE_LOG(LogTemp, Warning, TEXT("GameState could not get game mode to load a room manager"))
		}
	}
	
	Super::BeginPlay();
}

ARoomManager* ANetSessionGameState::GetRoomManager() const
{
	return RoomManager;
}

void ANetSessionGameState::OnRep_RoomManager()
{
	if(IsValid(RoomManager))
	{
		UE_LOG(LogTemp, Warning, TEXT("Game State Updated Room Manager Reference"));
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("Game State has an invalid Room Manager Reference"));
	}
}
