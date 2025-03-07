// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyPlayerState.h"

#include "SVSLogger.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyAttributeSet.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Players/SpyCharacter.h"
#include "Players/SpyHUD.h"
#include "Players/PlayerSaveGame.h"
#include "Players/SpyPlayerController.h"

ASpyPlayerState::ASpyPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<USpyAbilitySystemComponent>(
		TEXT("Ability System Component"));
	AbilitySystemComponent->SetIsReplicated(true);

	AttributeSet = CreateDefaultSubobject<USpyAttributeSet>("Attribute Set");

	SpyDeadTag = FGameplayTag::RequestGameplayTag("State.Dead");

	// TODO review
	/** Mixed mode means we only are replicated the GEs to ourself, not the GEs to
	 * simulated proxies. If another GDPlayerState (Hero) receives a GE, we won't
	 * be told about it by the Server. Attributes, GameplayTags, and
	 * GameplayCues will still replicate to us.
	 * AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	 *
	 * Set PlayerState's NetUpdateFrequency to the same as the Character.
	 * Default is very low for PlayerStates and introduces perceived lag in the ability system.
	 * 100 is probably way too high for a shipping game, you can adjust to fit your needs. */
	NetUpdateFrequency = 100.0f;
}

void ASpyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams PushedRepNotifyAlwaysParams;
	PushedRepNotifyAlwaysParams.bIsPushBased = true;
	PushedRepNotifyAlwaysParams.RepNotifyCondition = REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CurrentStatus, PushedRepNotifyAlwaysParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PlayerRemainingMatchTimeSeconds, PushedRepNotifyAlwaysParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SpyPlayerTeam, PushedRepNotifyAlwaysParams)
	// FDoRepLifetimeParams PushedRepNotifyParams;
	// PushedRepNotifyParams.bIsPushBased = true;
	// PushedRepNotifyParams.RepNotifyCondition = REPNOTIFY_Always;
	// PushedRepNotifyParams.Condition = COND_None;
	// DOREPLIFETIME_WITH_PARAMS_FAST(ASpyPlayerState, SpyPlayerTeam, PushedRepNotifyParams)
}

void ASpyPlayerState::BeginPlay()
{
	Super::BeginPlay();

	SetPlayerName(DefaultSpyName);
	
	/** Update player controller with pointer to self */
	if (ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(GetPlayerController()))
	{
		if (!IsValid(SpyPlayerController->GetSpyPlayerState()))
		{
			SpyPlayerController->SetSpyPlayerState(this);
			if (!IsRunningDedicatedServer())
			{ LoadSavedPlayerInfo(); }
		}
		SetCurrentStatus(EPlayerGameStatus::WaitingForStart);
	}

	/** Setup ability system component as Playerstate is the owner */
	if (AbilitySystemComponent)
	{
		/** Health Attribute change callback */
		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			AttributeSet->GetHealthAttribute()).AddUObject(this, &ASpyPlayerState::HealthChanged);
	}

	/** Listen for match start announcements */
	if (ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
	{ SpyGameState->OnStartMatchDelegate.AddUObject(this, &ThisClass::StartMatchForPlayer); }
	
}

UAbilitySystemComponent* ASpyPlayerState::GetAbilitySystemComponent() const
{
	return GetSpyAbilitySystemComponent();
}

USpyAttributeSet* ASpyPlayerState::GetAttributeSet() const
{
	return AttributeSet;
}

bool ASpyPlayerState::IsAlive() const
{
	return GetHealth() >= 1.0f;
}

float ASpyPlayerState::GetHealth() const
{
	return AttributeSet->GetHealth();
}

float ASpyPlayerState::GetMaxHealth() const
{
	return AttributeSet->GetMaxHealth();
}

void ASpyPlayerState::SetCurrentStatus(const EPlayerGameStatus PlayerGameStatus)
{
	checkfSlow(
		IsRunningDedicatedServer(),
		"ASpyPlayerState::SetCurrentStatus cannot be called by a client");
	
	CurrentStatus = PlayerGameStatus;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, CurrentStatus, this);

	UE_LOG(SVSLogDebug, Warning, TEXT("%i %s playerstate set currentstatus to %hhd"),
		GetPlayerId(), *GetPlayerName(), CurrentStatus)
	
	/** If Player is Ready then notify game mode */
	if (PlayerGameStatus == EPlayerGameStatus::Ready)
	{
		ASpyVsSpyGameMode* SvsGameMode = GetWorld()->GetAuthGameMode<ASpyVsSpyGameMode>();
		check(SvsGameMode);
		SvsGameMode->PlayerNotifyIsReady(this);
	}
}

void ASpyPlayerState::SetSpyPlayerTeam(const EPlayerTeam InSpyPlayerTeam)
{
	checkfSlow(
		IsRunningDedicatedServer(),
		"ASpyPlayerState::SetSpyPlayerTeam cannot be called by a client");
	
	SpyPlayerTeam = InSpyPlayerTeam;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SpyPlayerTeam, this);
	OnSpyTeamUpdate.Broadcast(SpyPlayerTeam);
}

void ASpyPlayerState::OnRep_SpyPlayerTeam() const
{
	OnSpyTeamUpdate.Broadcast(SpyPlayerTeam);
}

void ASpyPlayerState::StartMatchForPlayer(const float InMatchStartTime)
{
	if (IsRunningDedicatedServer())
	{ SetPlayerRemainingMatchTime(InMatchStartTime, false); }
	
	if (ASpyPlayerController* PlayerController = Cast<ASpyPlayerController>(GetPlayerController()))
	{ PlayerController->StartMatch(); }
}

void ASpyPlayerState::SetPlayerRemainingMatchTime(const float InMatchTimeLength, const bool bIncludeTimePenalty)
{
	checkfSlow(
		IsRunningDedicatedServer(),
		"ASpyPlayerState::SetPlayerRemainingMatchTime cannot be called by a client");
	
	/** Set match time or update it with a time penalty */
	if (!bIncludeTimePenalty)
	{ PlayerRemainingMatchTimeSeconds = InMatchTimeLength; }
	else
	{ PlayerRemainingMatchTimeSeconds -= PlayerMatchTimePenaltyInSeconds; }
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PlayerRemainingMatchTimeSeconds, this);

	/** Expire timer if it has run out */
	IsPlayerMatchTimeExpired() ? SetPlayerMatchTimeExpired() : UpdatePlayerMatchTimer();
}

void ASpyPlayerState::OnRep_PlayerRemainingMatchTimeSeconds() const
{
	OnPlayerMatchTimeUpdated.Broadcast(PlayerRemainingMatchTimeSeconds);
}

bool ASpyPlayerState::IsPlayerMatchTimeExpired() const
{
	checkfSlow(
		IsRunningDedicatedServer(),
		"ASpyPlayerState::IsPlayerRemainingMatchTimeExpired cannot be called by a client");

	const ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();

	return PlayerRemainingMatchTimeSeconds - SpyGameState->GetSpyMatchElapsedTime() <= 0.5f;
}

void ASpyPlayerState::UpdatePlayerMatchTimer()
{
	GetWorld()->GetTimerManager().SetTimer(
		PlayerMatchTimerHandle,
		this,
		&ThisClass::SetPlayerMatchTimeExpired,
		PlayerRemainingMatchTimeSeconds,
		false);
	
	UE_LOG(SVSLogDebug, Warning, TEXT("%s Character: %s playerstate set matchdeadline to: %f"),
		GetPawn()->IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName(),
		PlayerRemainingMatchTimeSeconds);
}

// float ASpyPlayerState::UpdateRemainingMatchTimeSeconds()
// {
// 	checkfSlow(
// 		IsRunningDedicatedServer(),
// 		"ASpyPlayerState::GetPlayerMatchSecondsRemaining cannot be run by clients");
//
// 	if (const ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
// 	{
// 		const float ElapsedTime = SpyGameState->GetSpyMatchElapsedTime();
// 		PlayerRemainingMatchTimeSeconds -= ElapsedTime;
//
// 		if (PlayerRemainingMatchTimeSeconds > 0.5f)
// 		{ return PlayerRemainingMatchTimeSeconds; }
// 	}
// 	return 0.0f;
// }

void ASpyPlayerState::SetPlayerMatchTimeExpired()
{
	UE_LOG(SVSLogDebug, Warning, TEXT("%hhd %i %s playerstate timer expired"),
		GetLocalRole(), GetLocalRole(), *GetPlayerName())
	ASpyCharacter* SpyCharacter = GetPawn<ASpyCharacter>();
	
	if (!IsValid(SpyCharacter) ||
		GetLocalRole() == ROLE_SimulatedProxy ||
		GetCurrentStatus() != EPlayerGameStatus::Playing)
	{ return; }

	if (IsRunningDedicatedServer())
	{
		if (ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
		{
			/** Player ran out of time so notify game that their match has ended */
			GetWorld()->GetTimerManager().ClearTimer(PlayerMatchTimerHandle);
			SpyGameState->RequestSubmitMatchResult(this, true);
			SpyCharacter->DisableSpyCharacter();
		}
		else
		{
			UE_LOG(SVSLog, Warning, TEXT(
				"Playerstate attempted PlayerMatchTimeExpired but GameState is null"));
		}
	}
	
	if (ASpyPlayerController* PlayerController = Cast<ASpyPlayerController>(GetPlayerController()))
	{ PlayerController->EndMatch(); }
}

void ASpyPlayerState::OnPlayerReachedEnd()
{
	/** just run this on the server */
	if (GetWorld()->GetNetMode() == NM_Client ||
		GetCurrentStatus() == EPlayerGameStatus::Finished ||
		GetCurrentStatus() == EPlayerGameStatus::WaitingForAllPlayersFinish)
	{ return; }
	
	if (ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
	{ SpyGameState->RequestSubmitMatchResult(this, false); }
}

void ASpyPlayerState::NM_EndMatch_Implementation()
{
	if (ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(GetPlayerController()))
	{ SpyPlayerController->EndMatch(); }
}

void ASpyPlayerState::OnRep_CurrentStatus()
{
	if (ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
	{
		SpyGameState->SetServerLobbyEntry(
			GetPlayerName(),
			BP_GetUniqueId(),
			GetCurrentStatus(),
			GetPingInMilliseconds());
	}
		
	if (!IsValid(GetPlayerController()) || IsRunningDedicatedServer())
	{ return; }
	
	UE_LOG(SVSLogDebug, Warning, TEXT("%hhd %s playerstate set onrep_currentstatus to %hhd"),
		GetLocalRole(), *GetPlayerName(), CurrentStatus)
	
	if (const ASpyHUD* PlayerHUD = Cast<ASpyHUD>(GetPlayerController()->GetHUD()))
	{ PlayerHUD->UpdateDisplayedPlayerStatus(CurrentStatus); }
}

void ASpyPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	if (const ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(
		GetPlayerController()))
	{ SpyPlayerController->OnPlayerStateReceived.Broadcast(); }

	if (ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
	{
		SpyGameState->SetServerLobbyEntry(
			GetPlayerName(),
			BP_GetUniqueId(),
			GetCurrentStatus(),
			GetPingInMilliseconds());
	}
		
	if (!IsValid(GetPlayerController()) || IsRunningDedicatedServer())
	{ return; }

	if (const ASpyHUD* PlayerHUD = Cast<ASpyHUD>(GetPlayerController()->GetHUD()))
	{ PlayerHUD->UpdateDisplayedPlayerStatus(CurrentStatus); }
}

void ASpyPlayerState::OnRep_PlayerId()
{
	Super::OnRep_PlayerId();
	UE_LOG(SVSLogDebug, Log, TEXT("Playerstate ID: %i Role: %hhd"), GetPlayerId(), GetLocalRole());
}

void ASpyPlayerState::OnDeactivated()
{
	if (ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
	{
		SpyGameState->SetServerLobbyEntry(
		"",
		BP_GetUniqueId(),
		EPlayerGameStatus::None,
		0.0f,
		true);
	}
	
	Super::OnDeactivated();
}

void ASpyPlayerState::SavePlayerDelegate(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Player Save Process: %s"), bSuccess ? *FString("Successful") : *FString("Failed"));
}

void ASpyPlayerState::LoadPlayerSaveDelegate(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData)
{
	if (const UPlayerSaveGame* LoadedPlayerInfo = Cast<UPlayerSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
	{
		GetPlayerController()->SetName(LoadedPlayerInfo->SpyPlayerName);
		OnSaveGameLoad.Broadcast();
	}
}

void ASpyPlayerState::SavePlayerInfo()
{
	if (IsRunningDedicatedServer())
	{ return; }
	
	if (UPlayerSaveGame* SaveGameInstance = Cast<UPlayerSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UPlayerSaveGame::StaticClass())))
	{
		FAsyncSaveGameToSlotDelegate OnSavedToSlot;
		OnSavedToSlot.BindUObject(this, &ThisClass::SavePlayerDelegate);

		/** Assign data to be saved */
		SaveGameInstance->SpyPlayerName = *GetPlayerName();
		SaveGameInstance->UserIndex = SaveUserIndex;
		
		UGameplayStatics::AsyncSaveGameToSlot(
			SaveGameInstance,
			SaveSlotName,
			SaveUserIndex,
			OnSavedToSlot);
	}
}

void ASpyPlayerState::LoadSavedPlayerInfo_Implementation()
{
	if (IsRunningDedicatedServer())
	{ return; }
	
	FAsyncLoadGameFromSlotDelegate OnLoadSaveFromSlot;
	OnLoadSaveFromSlot.BindUObject(this, &ThisClass::LoadPlayerSaveDelegate);
	
	UGameplayStatics::AsyncLoadGameFromSlot(
		SaveSlotName,
		SaveUserIndex,
		OnLoadSaveFromSlot);
}

void ASpyPlayerState::HealthChanged(const FOnAttributeChangeData& Data)
{
	const ASpyPlayerController* SpyController = Cast<ASpyPlayerController>(GetPlayerController());
	if (!IsValid(SpyController))
	{ return; }
	
	// UE_LOG(LogTemp, Warning, TEXT("%s Character: %s called attronchange: %s"),
	// 	SpyController->IsLocalController() ? *FString("True") : *FString("False"),
	// 	*SpyCharacter->GetName(),
	// 	*FString(Data.Attribute.AttributeName));

	if (SpyController->IsLocalController())
	{
		if (ASpyHUD* SpyHUD = Cast<ASpyHUD>(SpyController->GetHUD()))
		{
			SpyHUD->DisplayCharacterHealth(
				GetAttributeSet()->GetHealth(),
				GetAttributeSet()->GetMaxHealth());
		}
	}
	
	if (!IsRunningDedicatedServer() || CurrentStatus != EPlayerGameStatus::Playing)
	{ return; }
	
	/** Check for and handle knockdown and death */
	ASpyCharacter* SpyCharacter = GetPawn<ASpyCharacter>();
	if (IsValid(SpyCharacter) &&
		!IsAlive() &&
		!AbilitySystemComponent->HasMatchingGameplayTag(SpyDeadTag))
	{ SpyCharacter->RequestDeath(); }
}
