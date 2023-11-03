// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/SpyVsSpyGameState.h"

#include "SVSLogger.h"
#include "GameModes/SpyItemWorldSubsystem.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Items/InventoryComponent.h"
#include "Rooms/RoomManager.h"
#include "Engine/Public/Net/UnrealNetwork.h"
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
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SpyMatchState, SharedParamsRepNotifyAlwaysSkipOwner);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SVSGameType, SharedParamsRepNotifyAlwaysSkipOwner);

	FDoRepLifetimeParams SharedParamsRepNotifyAlways;
	SharedParamsRepNotifyAlways.bIsPushBased = true;
	SharedParamsRepNotifyAlways.RepNotifyCondition = REPNOTIFY_Always;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Results, SharedParamsRepNotifyAlways);

	FDoRepLifetimeParams SharedParamsRepNotifyChanged;
	SharedParamsRepNotifyChanged.bIsPushBased = true;
	SharedParamsRepNotifyChanged.RepNotifyCondition = REPNOTIFY_OnChanged;
	
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SpyMatchStartTime, SharedParamsRepNotifyChanged);
}

void ASpyVsSpyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	for (APlayerState* PlayerStateToRep : PlayerArray)
	{
		if (ASpyPlayerState* SpyPlayerStateToRep = Cast<ASpyPlayerState>(PlayerStateToRep))
		{ SpyPlayerStateToRep->OnRep_CurrentStatus(); }
	}
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

void ASpyVsSpyGameState::SetSpyMatchState(const ESpyMatchState InSpyMatchState)
{
	OldSpyMatchState = SpyMatchState;
	SpyMatchState = InSpyMatchState;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SpyMatchState, this);
	
	/** Manual invocation of OnRep_GameState so server will also run the method */
	if (HasAuthority())
	{ OnRep_SpyMatchState(); }
}

void ASpyVsSpyGameState::OnRep_SpyMatchState() const
{
}

void ASpyVsSpyGameState::SetAllPlayerGameStatus(const EPlayerGameStatus InPlayerGameStatus)
{
	if (!HasAuthority())
	{ return; }
	
	/** Update each player status */
	for (APlayerState* PlayerState : PlayerArray)
	{
		ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(PlayerState);
		if (IsValid(SpyPlayerState) && !SpyPlayerState->IsSpectator())
		{ SpyPlayerState->SetCurrentStatus(InPlayerGameStatus); }
	}
}

void ASpyVsSpyGameState::SetServerLobbyEntry(const FString InPLayerName,
	const FUniqueNetIdRepl& PlayerStateUniqueId, const EPlayerGameStatus SpyPLayerCurrentStatus, const float Ping, const bool bRemoveEntry)
{
	if (!PlayerStateUniqueId.IsValid())
	{ return; }
	
	if (!bRemoveEntry)
	{
		FServerLobbyEntry LobbyEntry = FServerLobbyEntry(InPLayerName, SpyPLayerCurrentStatus, Ping);
		ServerLobbyEntries.Emplace(PlayerStateUniqueId, LobbyEntry);
	}
	else
	{ ServerLobbyEntries.Remove(PlayerStateUniqueId); }

	OnServerLobbyUpdate.Broadcast();
}

void ASpyVsSpyGameState::GetServerLobbyEntry(TArray<FServerLobbyEntry>& LobbyListings)
{
	ServerLobbyEntries.GenerateValueArray(LobbyListings);
}

void ASpyVsSpyGameState::SetGameType(const ESVSGameType InGameType)
{
	SVSGameType = InGameType;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SVSGameType, this);
	
	/** Manual invocation of OnRep_GameState so server will also run the method */
	if (HasAuthority())
	{ OnRep_SVSGameType(); }
}

void ASpyVsSpyGameState::OnRep_SVSGameType() const
{
	/** If Replicated with COND_SkipOwner then Authority will need to run this manually */
	OnGameTypeUpdateDelegate.Broadcast(SVSGameType);
}

void ASpyVsSpyGameState::SetSpyMatchTimeLength(const float InSecondsTotal)
{
	SpyMatchTimeLength = InSecondsTotal;
}

void ASpyVsSpyGameState::SetSpyMatchStartTime(const float InMatchStartTime)
{
	SpyMatchStartTime = InMatchStartTime;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SpyMatchStartTime, this);
}

void ASpyVsSpyGameState::NM_MatchStart_Implementation()
{
	ClearResults();
	SetSpyMatchStartTime(GetServerWorldTimeSeconds());
	SetSpyMatchState(ESpyMatchState::Playing);
	UpdatePlayerStateWithMatchTimeLength(); // Here or delegate?
	OnStartMatchDelegate.Broadcast(SpyMatchStartTime);
	UE_LOG(SVSLogDebug, Log, TEXT("Gamestate match start time: %f"), SpyMatchStartTime);
}

void ASpyVsSpyGameState::UpdatePlayerStateWithMatchTimeLength()
{
	if (!IsValid(GetWorld()->GetAuthGameMode()))
	{ return; }
	
	for (APlayerState* PlayerState : PlayerArray)
	{
		if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(PlayerState))
		{ SpyPlayerState->SetPlayerRemainingMatchTime(SpyMatchTimeLength); }
		else
		{ UE_LOG(SVSLogDebug, Log, TEXT("SpyGameState could not set player match time")); }
	}
}

void ASpyVsSpyGameState::RequestSubmitMatchResult(ASpyPlayerState* InSpyPlayerState, bool bPlayerTimeExpired)
{
	FGameResult Result;
	Result.Time = GetSpyMatchElapsedTime();
	Result.Name = InSpyPlayerState->GetPlayerName();
	
	bool IsWinner = false;
	if (!bPlayerTimeExpired)
	{
		IsWinner = Results.Num() == 0;
		Result.bCompletedMission = true;
	}
	InSpyPlayerState->SetIsWinner(IsWinner);
	Result.bIsWinner = IsWinner;
	
	Results.Add(Result);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Results, this);
	TryFinaliseScoreBoard();
}

void ASpyVsSpyGameState::TryFinaliseScoreBoard()
{
	if(CheckAllResultsIn())
	{
		/** Update each player status */
		SetAllPlayerGameStatus(EPlayerGameStatus::Finished);
		
		/** Results Replication Is Pushed to Mark Dirty */
		SpyMatchState = ESpyMatchState::GameOver;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Results, this);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SpyMatchState, this);
		
		/** if running a local game then need to call the OnRep function manually */
		if (HasAuthority() && !IsRunningDedicatedServer())
		{ OnRep_ResultsUpdated(); }
	}
}

void ASpyVsSpyGameState::OnRep_ResultsUpdated()
{
	for (const TObjectPtr<APlayerState> PlayerState : PlayerArray)
	{
		if(ASpyPlayerController* PlayerController = Cast<ASpyPlayerController>(
			PlayerState->GetPlayerController()))
		{ PlayerController->RequestUpdatePlayerResults(); }
	}
}

bool ASpyVsSpyGameState::CheckAllResultsIn() const
{
	const uint8 FinalNumPlayers = PlayerArray.Num();
	if (FinalNumPlayers > 0 && Results.Num() == FinalNumPlayers)
	{ return true; }
	
	UE_LOG(SVSLog, Log, TEXT("GameState CheckAllResults has %i Players and %i Results."), FinalNumPlayers, Results.Num());
	return false;
}

void ASpyVsSpyGameState::ClearResults()
{
	Results.Empty();
}

ARoomManager* ASpyVsSpyGameState::GetRoomManager() const
{
	return RoomManager;
}

void ASpyVsSpyGameState::SetRequiredMissionItems(const TArray<UInventoryBaseAsset*>& InRequiredMissionItems)
{
	if (!HasAuthority())
	{ return; }

	RequiredMissionItems = InRequiredMissionItems;
}

void ASpyVsSpyGameState::GetRequiredMissionItems(TArray<UInventoryBaseAsset*>& RequestedRequiredMissionItems)
{
	RequestedRequiredMissionItems = RequiredMissionItems;
}

void ASpyVsSpyGameState::OnRep_RoomManager()
{
	if(IsValid(RoomManager))
	{ UE_LOG(SVSLogDebug, Log, TEXT("Game State Updated Room Manager Reference"));
	}
	else
	{ UE_LOG(SVSLogDebug, Log, TEXT("Game State has an invalid Room Manager Reference")); }
}

// void ASpyVsSpyGameState::OnPlayerReachedEnd(ASpyCharacter* InSpyCharacter)
// {
// 	if (!HasAuthority() ||
// 		!IsValid(InSpyCharacter) ||
// 		!IsValid(InSpyCharacter->GetPlayerInventoryComponent()) )
// 	{ return; }
// 	
// 	TArray<UInventoryBaseAsset*> PlayerInventory;
// 	InSpyCharacter->GetPlayerInventoryComponent()->GetInventoryItems(PlayerInventory);
// 	if (PlayerInventory.Num() < 1)
// 	{ return; }
// 	
// 	for (UInventoryBaseAsset* MissionItem : RequiredMissionItems)
// 	{
// 		if (!PlayerInventory.Contains(MissionItem))
// 		{ return; }
// 	}
// 	
// 	if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(InSpyCharacter))
// 	{ PlayerRequestSubmitResults(SpyPlayerState); }
// 	
// 	InSpyCharacter->NM_FinishedMatch();
// 	TryFinaliseScoreBoard();
// }

// void ASpyVsSpyGameState::NotifyPlayerTimeExpired(ASpyCharacter* InSpyCharacter)
// {
// 	if (!HasAuthority() || !IsValid(InSpyCharacter))
// 	{ return; }
//
// 	if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(InSpyCharacter))
// 	{ PlayerRequestSubmitResults(SpyPlayerState, true); }
//
// 	InSpyCharacter->NM_FinishedMatch();
// 	TryFinaliseScoreBoard();
// }
