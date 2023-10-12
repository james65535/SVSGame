// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyPlayerState.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyAttributeSet.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Players/SpyCharacter.h"
#include "Players/SpyHUD.h"
#include "Players/SpyPlayerController.h"

ASpyPlayerState::ASpyPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<USpyAbilitySystemComponent>(TEXT("Ability System Component"));
	AbilitySystemComponent->SetIsReplicated(true);

	AttributeSet = CreateDefaultSubobject<USpyAttributeSet>("Attribute Set");

	SpyDeadTag = FGameplayTag::RequestGameplayTag("State.Dead");

	// TODO review
	// Mixed mode means we only are replicated the GEs to ourself, not the GEs to simulated proxies. If another GDPlayerState (Hero) receives a GE,
	// we won't be told about it by the Server. Attributes, GameplayTags, and GameplayCues will still replicate to us.
	//AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Set PlayerState's NetUpdateFrequency to the same as the Character.
	// Default is very low for PlayerStates and introduces perceived lag in the ability system.
	// 100 is probably way too high for a shipping game, you can adjust to fit your needs.
	NetUpdateFrequency = 100.0f;
}

void ASpyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ASpyPlayerState, CurrentStatus, COND_None, REPNOTIFY_Always);
}

void ASpyPlayerState::BeginPlay()
{
	Super::BeginPlay();

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
		if(const ASpyHUD* PlayerHUD = Cast<ASpyHUD>(SpyPlayerController->GetHUD()))
		{
			PlayerHUD->UpdateDisplayedPlayerState(CurrentStatus);
		}
	}
}

void ASpyPlayerState::OnRep_CurrentStatus()
{
	if(const ASpyHUD* PlayerHud = Cast<ASpyHUD>(GetPlayerController()->GetHUD()))
	{
		PlayerHud->UpdateDisplayedPlayerState(CurrentStatus);
	}
}

void ASpyPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	if (const ASpyPlayerController* SpyPlayerController =  Cast<ASpyPlayerController>(GetPlayerController()))
	{
		SpyPlayerController->OnPlayerStateReceived.Broadcast();
	}
}

void ASpyPlayerState::SavePlayerInfo()
{
	// TODO Implement
}

void ASpyPlayerState::LoadSavedPlayerInfo_Implementation()
{
	// TODO Implement
}

void ASpyPlayerState::HealthChanged(const FOnAttributeChangeData& Data)
{
	// Check for and handle knockdown and death
	ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetPawn());
	if (IsValid(SpyCharacter) && !IsAlive() && !AbilitySystemComponent->HasMatchingGameplayTag(SpyDeadTag))
	{
		SpyCharacter->FinishDying();
	}
}
