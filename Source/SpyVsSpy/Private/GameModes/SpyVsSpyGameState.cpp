// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/SpyVsSpyGameState.h"

#include "SVSLogger.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Items/InventoryComponent.h"
#include "Rooms/RoomManager.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Players/SpyPlayerController.h"
#include "Players/SpyCharacter.h"

ASpyVsSpyGameState::ASpyVsSpyGameState()
{
	bReplicates = true;
}

void ASpyVsSpyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParamsRepNotifyAlwaysSkipOwner;
	SharedParamsRepNotifyAlwaysSkipOwner.bIsPushBased = true;
	SharedParamsRepNotifyAlwaysSkipOwner.RepNotifyCondition = REPNOTIFY_Always;
	SharedParamsRepNotifyAlwaysSkipOwner.Condition = COND_SkipOwner;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RoomManager, SharedParamsRepNotifyAlwaysSkipOwner);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SpyGameState, SharedParamsRepNotifyAlwaysSkipOwner);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SVSGameType, SharedParamsRepNotifyAlwaysSkipOwner);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Results, SharedParamsRepNotifyAlwaysSkipOwner);

	FDoRepLifetimeParams SharedParamsRepNotifyChanged;
	SharedParamsRepNotifyChanged.bIsPushBased = true;
	SharedParamsRepNotifyChanged.RepNotifyCondition = REPNOTIFY_OnChanged;
	
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PlayerMatchStartTime, SharedParamsRepNotifyChanged);
}

void ASpyVsSpyGameState::SetGameState(const ESpyMatchState InGameState)
{
	OldSpyGameState = SpyGameState;
	SpyGameState = InGameState;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SpyGameState, this);
	
	/** Manual invocation of OnRep_GameState so server will also run the method */
	if (HasAuthority())
	{ OnRep_GameState(); }
}

void ASpyVsSpyGameState::SetGameType(const ESVSGameType InGameType)
{
	SVSGameType = InGameType;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SVSGameType, this);
	
	/** Manual invocation of OnRep_GameState so server will also run the method */
	if (HasAuthority())
	{ OnRep_GameType(); }
}

void ASpyVsSpyGameState::ClearResults()
{
	Results.Empty();
}

void ASpyVsSpyGameState::NM_MatchStart_Implementation()
{
	ClearResults();
	MatchStartTime = GetServerWorldTimeSeconds();
	SetGameState(ESpyMatchState::Playing);
	OnStartMatchDelegate.Broadcast(MatchStartTime);
	UpdatePlayerStateMatchTime();
	UE_LOG(SVSLogDebug, Log, TEXT("Gamestate match start time: %f"), MatchStartTime);
}

void ASpyVsSpyGameState::OnRep_ResultsUpdated()
{
	for (const TObjectPtr<APlayerState> PlayerState : PlayerArray)
	{
		if(const ASpyPlayerController* PlayerController = Cast<ASpyPlayerController>(
			PlayerState->GetPlayerController()))
		{ PlayerController->RequestDisplayFinalResults(); }
	}
}

void ASpyVsSpyGameState::PlayerRequestSubmitResults(const ASpyCharacter* InSpyCharacter)
{
	if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(InSpyCharacter->GetPlayerState()))
	{
		if (SpyPlayerState->GetCurrentState() != EPlayerGameStatus::Finished)
		{
			SpyPlayerState->SetCurrentStatus(EPlayerGameStatus::Finished);
			
			FGameResult Result;
			Result.Time = GetServerWorldTimeSeconds() - MatchStartTime;
			Result.Name = SpyPlayerState->GetPlayerName();
			const bool IsWinner = Results.Num() == 0;
			SpyPlayerState->SetIsWinner(IsWinner);
			Result.bIsWinner = IsWinner;
			Results.Add(Result);
		}
	}
}

void ASpyVsSpyGameState::TryFinaliseScoreBoard()
{
	// if(CheckAllResultsIn())
	// {
		/** Results Replication Is Pushed to Mark Dirty */
		SpyGameState = ESpyMatchState::GameOver;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Results, this);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SpyGameState, this);
		
		/** if running a local game then need to call the OnRep function manually */
		if (HasAuthority() && !IsRunningDedicatedServer())
		{ OnRep_ResultsUpdated(); }
	//}
}

bool ASpyVsSpyGameState::CheckAllResultsIn() const
{
	const uint8 FinalNumPlayers = PlayerArray.Num();
	if (FinalNumPlayers > 0 && Results.Num() == FinalNumPlayers)
	{ return true; }
	
	UE_LOG(SVSLog, Log, TEXT("GameState CheckAllResults has %i Players and %i Results."), FinalNumPlayers, Results.Num());
	return false;
}

void ASpyVsSpyGameState::OnRep_GameState() const
{
}

void ASpyVsSpyGameState::OnRep_GameType() const
{
	/** If Replicated with COND_SkipOwner then Authority will need to run this manually */
	OnGameTypeUpdateDelegate.Broadcast(SVSGameType);
}

void ASpyVsSpyGameState::BeginPlay()
{
	/** Load RoomManager and persist reference for replication to clients */
	if (GetLocalRole() == ROLE_Authority && !IsValid(RoomManager))
	{
		if(ASpyVsSpyGameMode* GameMode = Cast<ASpyVsSpyGameMode>(AuthorityGameMode))
		{
			RoomManager = GameMode->LoadRoomManager();
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RoomManager, this);
		}
		if (!IsValid(RoomManager))
		{ UE_LOG(SVSLog, Warning, TEXT("GameState could not get game mode to load a room manager")); }
	}
	
	Super::BeginPlay();
}

ARoomManager* ASpyVsSpyGameState::GetRoomManager() const
{
	return RoomManager;
}

void ASpyVsSpyGameState::SetPlayerMatchTime(const float InMatchStartTime)
{
	PlayerMatchStartTime = InMatchStartTime;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PlayerMatchStartTime, this);
}

void ASpyVsSpyGameState::SetRequiredMissionItems(const TArray<UInventoryBaseAsset*>& InRequiredMissionItems)
{
	if (!HasAuthority())
	{ return; }

	RequiredMissionItems = InRequiredMissionItems;
}

void ASpyVsSpyGameState::OnPlayerReachedEnd(ASpyCharacter* InSpyCharacter)
{
	
	if (!HasAuthority() ||
		!IsValid(InSpyCharacter) ||
		!InSpyCharacter->GetPlayerInventoryComponent()->IsValidLowLevelFast())
	{ return; }
	
	TArray<UInventoryBaseAsset*> PlayerInventory;
	InSpyCharacter->GetPlayerInventoryComponent()->GetInventoryItems(PlayerInventory);

	if (PlayerInventory.Num() < 1)
	{ return; }
	
	for (UInventoryBaseAsset* MissionItem : RequiredMissionItems)
	{
		if (!PlayerInventory.Contains(MissionItem))
		{ return; }
	}

	PlayerRequestSubmitResults(InSpyCharacter);
	InSpyCharacter->NM_FinishedMatch();
	TryFinaliseScoreBoard();
}

void ASpyVsSpyGameState::UpdatePlayerStateMatchTime()
{
	if (!HasAuthority())
	{ return; }
	
	for (APlayerState* PlayerState : PlayerArray)
	{
		if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(PlayerState))
		{ SpyPlayerState->SetPlayerRemainingMatchTime(PlayerMatchStartTime); }
		else
		{ UE_LOG(SVSLogDebug, Log, TEXT("SpyGameState could not set player match time")); }
	}
}

void ASpyVsSpyGameState::OnRep_RoomManager()
{
	if(IsValid(RoomManager))
	{ UE_LOG(SVSLogDebug, Log, TEXT("Game State Updated Room Manager Reference"));
	}
	else
	{ UE_LOG(SVSLogDebug, Log, TEXT("Game State has an invalid Room Manager Reference")); }
}
