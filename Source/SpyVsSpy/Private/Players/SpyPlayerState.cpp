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
#include "Items/InventoryComponent.h"

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
	DOREPLIFETIME_WITH_PARAMS_FAST(ASpyPlayerState, CurrentStatus, PushedRepNotifyAlwaysParams);

	FDoRepLifetimeParams PushedRepNotifyParams;
	PushedRepNotifyParams.bIsPushBased = true;
	PushedRepNotifyParams.RepNotifyCondition = REPNOTIFY_OnChanged;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASpyPlayerState, PlayerRemainingMatchTime, PushedRepNotifyParams);
}

void ASpyPlayerState::BeginPlay()
{
	Super::BeginPlay();

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
	if (!HasAuthority())
	{ return; }
	
	CurrentStatus = PlayerGameStatus;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, CurrentStatus, this);

	/** If Player is Ready then notify game mode */
	if (PlayerGameStatus == EPlayerGameStatus::Ready)
	{
		ASpyVsSpyGameMode* SVSGameMode = Cast<ASpyVsSpyGameMode>(GetWorld()->GetAuthGameMode());
		check(SVSGameMode);
		SVSGameMode->PlayerNotifyIsReady(this);
	}
}

void ASpyPlayerState::SetPlayerRemainingMatchTime(const float InMatchTimeLength, const bool bIncludeTimePenalty)
{
	/** Apply time penalty if method was requested with penalty flag */
	if (bIncludeTimePenalty)
	{ PlayerRemainingMatchTime = GetPlayerRemainingMatchTime() - PlayerMatchTimePenaltyInSeconds; }
	else
	{ PlayerRemainingMatchTime = InMatchTimeLength; }
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PlayerRemainingMatchTime, this);

	if (IsPlayerRemainingMatchTimeExpired())
	{ PlayerMatchTimeExpired(); }
	else
	{ SetPlayerMatchTimer(); }
}

bool ASpyPlayerState::IsPlayerRemainingMatchTimeExpired() const
{
	const ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();
	if (GetWorld()->GetNetMode() == NM_Client || !IsValid(SpyGameState))
	{ return true; }
	
	if (GetPlayerRemainingMatchTime() - SpyGameState->GetSpyMatchElapsedTime() <= 0.0f)
	{ return true; }
	return false;
}

float ASpyPlayerState::GetPlayerRemainingMatchTime() const
{
	return PlayerRemainingMatchTime;
}

void ASpyPlayerState::SetPlayerMatchTimer()
{
	const ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();
	if (GetWorld()->GetNetMode() == NM_Client ||
		!IsValid(SpyGameState) ||
		GetCurrentStatus() != EPlayerGameStatus::Playing)
	{ return; }

	const float ElapsedTime = SpyGameState->GetSpyMatchElapsedTime();
	const float MatchPlayerTimeLeft = GetPlayerRemainingMatchTime() - ElapsedTime;
	
	UE_LOG(SVSLog, Warning, TEXT("%s Character: %s playerstate set matchdeadline to: %f"),
		GetPawn()->IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName(),
		GetPlayerSpyMatchSecondsRemaining()); //MatchPlayerTimeLeft);

	if (MatchPlayerTimeLeft >= 1.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			PlayerMatchTimerHandle,
			this,
			&ThisClass::PlayerMatchTimeExpired,
			GetPlayerSpyMatchSecondsRemaining(), //MatchPlayerTimeLeft,
			false);
	}
}

float ASpyPlayerState::GetPlayerSpyMatchSecondsRemaining() const
{
	if (const ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
	{
		const float ElapsedTime = SpyGameState->GetSpyMatchElapsedTime();
		const float MatchPlayerTimeLeft = GetPlayerRemainingMatchTime() - ElapsedTime;
		return MatchPlayerTimeLeft;
	}
	return 0.0f;
}

void ASpyPlayerState::PlayerMatchTimeExpired()
{
	ASpyCharacter* SpyCharacter = GetPawn<ASpyCharacter>();
	
	/** just run this on the server */
	if (!IsValid(SpyCharacter) ||
		GetWorld()->GetNetMode() == NM_Client ||
		GetCurrentStatus() != EPlayerGameStatus::Playing)
	{ return; }
	
	UE_LOG(SVSLog, Warning, TEXT("%s Character: %s playerstate trying matchtimeexpired with time: %f"),
		GetPawn()->IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName(),
		GetPlayerSpyMatchSecondsRemaining());
	
	/** Player ran out of time so notify game that their match has ended */
	GetWorld()->GetTimerManager().ClearTimer(PlayerMatchTimerHandle);
	
	if (ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>())
	{
		SetCurrentStatus(EPlayerGameStatus::WaitingForAllPlayersFinish);
		SpyGameState->RequestSubmitMatchResult(this, true);
		SpyCharacter->NM_FinishedMatch();

		// TODO probably don't need to actually destroy, character sets self to hidden and disables controls
		// if(!Destroy())
		// {
		// 	UE_LOG(SVSLog, Warning, TEXT(
		// 		"%s Character %s was not able to be destroyed by playerstate when time expired"),
		// 		GetPawn()->IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		// 		*GetName());
		// }
	}
	else
	{
		UE_LOG(SVSLog, Warning, TEXT(
			"Playerstate attempted PlayerMatchTimeExpired but could not get a valid pointer to game state"));
	}
}

void ASpyPlayerState::OnPlayerReachedEnd()
{
	ASpyCharacter* SpyCharacter = GetPawn<ASpyCharacter>();
	
	/** just run this on the server */
	if (!IsValid(SpyCharacter) ||
		GetWorld()->GetNetMode() == NM_Client ||
		GetCurrentStatus() == EPlayerGameStatus::Finished ||
		GetCurrentStatus() == EPlayerGameStatus::WaitingForAllPlayersFinish)
	{ return; }
	
	TArray<UInventoryBaseAsset*> PlayerInventory;
	SpyCharacter->GetPlayerInventoryComponent()->GetInventoryItems(PlayerInventory);
	ASpyVsSpyGameState* SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();
	if (IsValid(SpyGameState) && PlayerInventory.Num() >= 1)
	{
		TArray<UInventoryBaseAsset*> RequiredMissionItems;
		SpyGameState->GetRequiredMissionItems(RequiredMissionItems);
		for (UInventoryBaseAsset* MissionItem : RequiredMissionItems)
		{
			if (!PlayerInventory.Contains(MissionItem))
			{ return; }
		}
		SetCurrentStatus(EPlayerGameStatus::WaitingForAllPlayersFinish);
		SpyGameState->RequestSubmitMatchResult(this, false);
		SpyCharacter->NM_FinishedMatch();
	}
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

	if (const ASpyHUD* PlayerHUD = Cast<ASpyHUD>(GetPlayerController()->GetHUD()))
	{ PlayerHUD->UpdateDisplayedPlayerStatus(CurrentStatus); }
}

void ASpyPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	if (const ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(
		GetPlayerController()))
	{ SpyPlayerController->OnPlayerStateReceived.Broadcast(); }
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
