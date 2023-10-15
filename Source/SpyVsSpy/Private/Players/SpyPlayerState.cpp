// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyPlayerState.h"

#include "SVSLogger.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyAttributeSet.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "net/UnrealNetwork.h"
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

	FDoRepLifetimeParams PushedRepNotifyParams;
	PushedRepNotifyParams.bIsPushBased = true;
	PushedRepNotifyParams.RepNotifyCondition = REPNOTIFY_OnChanged;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASpyPlayerState, CurrentStatus, PushedRepNotifyParams);
}

void ASpyPlayerState::BeginPlay()
{
	Super::BeginPlay();

	/** Update player controller with pointer to self */
	if (ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(GetPlayerController()))
	{
		if (!IsValid(SpyPlayerController->SpyPlayerState))
		{
			SpyPlayerController->SetSpyPlayerState(this);
			if (!IsRunningDedicatedServer())
			{ LoadSavedPlayerInfo(); }
		}
		SetCurrentStatus(EPlayerGameStatus::Waiting);
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
	if (!HasAuthority()){ return; }
	
	CurrentStatus = PlayerGameStatus;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASpyPlayerState, CurrentStatus, this);

	/** If Player is Ready then notify game mode */
	if (PlayerGameStatus == EPlayerGameStatus::Ready)
	{
		ASpyVsSpyGameMode* SVSGameMode = Cast<ASpyVsSpyGameMode>(GetWorld()->GetAuthGameMode());
		check(SVSGameMode);
		SVSGameMode->PlayerNotifyIsReady(this);
	}
	
	if(const ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(GetOwner()))
	{
		if (const ASpyHUD* PlayerHUD = Cast<ASpyHUD>(SpyPlayerController->GetHUD()))
		{ PlayerHUD->UpdateDisplayedPlayerState(CurrentStatus); }
	}
}

void ASpyPlayerState::OnRep_CurrentStatus()
{
	if (!IsValid(GetPlayerController()))
	{
		UE_LOG(SVSLogDebug, Log, TEXT("Spy playerstate cannot get player controller"));
		return;
	}
	
	if (const ASpyHUD* PlayerHUD = Cast<ASpyHUD>(GetPlayerController()->GetHUD()))
	{ PlayerHUD->UpdateDisplayedPlayerState(CurrentStatus); }
}

void ASpyPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	if (const ASpyPlayerController* SpyPlayerController =  Cast<ASpyPlayerController>(
		GetPlayerController()))
	{ SpyPlayerController->OnPlayerStateReceived.Broadcast(); }
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
	/** Check for and handle knockdown and death */
	ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn());
	if (IsValid(SpyCharacter) &&
		!IsAlive() &&
		!AbilitySystemComponent->HasMatchingGameplayTag(SpyDeadTag))
	{ SpyCharacter->FinishDying(); }
}
