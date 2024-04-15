// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/SpyVsSpyGameState.h"

#include "SVSLogger.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Items/InventoryComponent.h"
#include "Rooms/RoomManager.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Players/SpyPlayerController.h"
#include "Players/SpyCharacter.h"

ASpyVsSpyGameState::ASpyVsSpyGameState()
{
	bReplicates = true;
	RoomManager = nullptr;
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
	// TODO we don't need to replicate room manager - remove
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
		{ UE_LOG(SVSLog, Warning, TEXT("SpyGameState could not set player match time")); }
	}
}

void ASpyVsSpyGameState::RequestSubmitMatchResult(ASpyPlayerState* InSpyPlayerState, const bool bPlayerTimeExpired)
{
	FGameResult Result;
	Result.Time = GetSpyMatchElapsedTime();
	Result.Name = InSpyPlayerState->GetPlayerName();

	if (InSpyPlayerState->GetCurrentStatus() == EPlayerGameStatus::WaitingForAllPlayersFinish)
	{
		/** Process results for player since match has a winner */
		Results.Add(Result);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Results, this);
		return;
	}
	
	const bool bMatchWinner = CheckSpyCompleteMission(InSpyPlayerState);
	if (bPlayerTimeExpired)
	{
		/** Process results for player since they ran out of time */
		InSpyPlayerState->SetCurrentStatus(EPlayerGameStatus::MatchTimeExpired);
	}
	else if (bMatchWinner && Results.Num() == 0)
	{
		/** Process player results and mark as winner of the match */
		InSpyPlayerState->SetCurrentStatus(EPlayerGameStatus::Finished);
		Result.bCompletedMission = true;
		InSpyPlayerState->SetIsWinner(bMatchWinner);
		Result.bIsWinner = bMatchWinner;
		Results.Add(Result);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Results, this);
		FinaliseMatchEnd();
	} 


}

void ASpyVsSpyGameState::FinaliseMatchEnd()
{
	for (APlayerState* PlayerState : PlayerArray)
	{
		if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(PlayerState))
		{
			/** Process match results for all non winners */
			const EPlayerGameStatus SpyPlayerGameStatus = SpyPlayerState->GetCurrentStatus();
			if (SpyPlayerGameStatus == EPlayerGameStatus::Playing || SpyPlayerGameStatus == EPlayerGameStatus::MatchTimeExpired)
			{
				SpyPlayerState->SetCurrentStatus(EPlayerGameStatus::WaitingForAllPlayersFinish);
				RequestSubmitMatchResult(SpyPlayerState, true);
			}

			SpyPlayerState->NM_EndMatch();
		}
	}

	TryFinaliseScoreBoard();
}

bool ASpyVsSpyGameState::CheckSpyCompleteMission(const ASpyPlayerState* SpyPlayerState) const
{
	if (!IsValid(SpyPlayerState))
	{ return false; }

	if (const ASpyCharacter* SpyCharacter = SpyPlayerState->GetPawn<ASpyCharacter>())
	{
		TArray<UInventoryBaseAsset*> PlayerInventory;
		SpyCharacter->GetPlayerInventoryComponent()->GetInventoryItems(PlayerInventory);
		if (PlayerInventory.Num() > 0)
		{
			for (UInventoryBaseAsset* MissionItem : RequiredMissionItems)
			{
				if (!PlayerInventory.Contains(MissionItem))
				{ return false; }
			}
			/** Player has required mission items */
			return true;
		}
	}
	return false;
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
	if(!IsValid(RoomManager) && HasAuthority())
	{ UE_LOG(SVSLogDebug, Log, TEXT("Game State has an invalid Room Manager Reference")); }
}
