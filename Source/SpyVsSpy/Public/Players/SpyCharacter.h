// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Players/SpyCombatantInterface.h"
#include "SpyCharacter.generated.h"

class USpyAbilitySystemComponent;
class ASpyPlayerState;
class ASVSRoom;
class UPlayerInventoryComponent;
class UPlayerInteractionComponent;
class UGameplayEffect;
class USpyGameplayAbility;
class USpyAttributeSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, ASpyCharacter*, Character);

UCLASS()
class SPYVSSPY_API ASpyCharacter : public ACharacter, public IAbilitySystemInterface, public ISpyCombatantInterface
{
	GENERATED_BODY()

public:
	
	ASpyCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	ASpyPlayerState* GetSpyPlayerState() const;
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	virtual void UpdateCameraLocation(const ASVSRoom* InRoom) const;
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	UPlayerInventoryComponent* GetPlayerInventoryComponent() const { return PlayerInventoryComponent; };
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	UPlayerInteractionComponent* GetPlayerInteractionComponent() const { return PlayerInteractionComponent; };

	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	virtual void SetSpyHidden(const bool bIsSpyHidden);

#pragma region="Health"
	// TODO Implement
	// UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	//UPlayerHealthComponent* GetPlayerHealthComponent() const { return PlayerHealthComponent; }

	UPROPERTY(BlueprintAssignable, Category = "SVS|Character")
	FCharacterDiedDelegate OnCharacterDied;
	
	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	virtual void FinishDying();

	/** Can only be called by the Server. Removing on the Server will remove from Client too */
	virtual void RemoveCharacterAbilities();

	UFUNCTION(BlueprintCallable, Category = "SVS|Character|Attributes")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "SVS|Character|Attributes")
	float GetMaxHealth() const;
#pragma endregion="Health"

private:

	UPROPERTY()
	ASpyPlayerState* SpyPlayerState;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UIsoCameraComponent* FollowCamera;

#pragma region="Inventory"
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "SVS|Character")
	UPlayerInventoryComponent* PlayerInventoryComponent;
#pragma endregion="Inventory"

#pragma region="Interaction"
	// TODO Implement
	// UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS|Character")
	// UPlayerHealthComponent* PlayerHealthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "SVS|Character")
	UPlayerInteractionComponent* PlayerInteractionComponent;
#pragma endregion="Interaction"

#pragma region="CharacterVisibility"
	/** Controls whether the character is visible to other players */
	UPROPERTY(ReplicatedUsing=OnRep_bIsHiddenInGame)
	bool bIsHiddenInGame = true;
	UFUNCTION()
	void OnRep_bIsHiddenInGame() { SetActorHiddenInGame(bIsHiddenInGame); }
#pragma endregion="CharacterVisibility"
	
#pragma region="MovementControls"
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;
	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;
	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SprintAction;
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;
	/** PrimaryAttack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PrimaryAttackAction;
	/** NextTrap Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* NextTrapAction;
	/** PrevTrap Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PrevTrapAction;
	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;
#pragma endregion="MovementControls"
	
protected:

#pragma region="ClassOverrides"
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
#pragma endregion="ClassOverrides"

	UFUNCTION()
	void OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor);
	UFUNCTION()
	void OnOverlapEnd(class AActor* OverlappedActor, class AActor* OtherActor);

#pragma region ="RoomTraversal"
	/** State and Logic for processing when character enters and leaves rooms */
	/** Track  which room the character is currently located in */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SVS|Room", meta = (AllowPrivateAccess = "true"))
	ASVSRoom* CurrentRoom;
	/** A transitory variable to track the new room during the process of character going from one room to another */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SVS|Room", meta = (AllowPrivateAccess = "true"))
	ASVSRoom* RoomEntering;
	/** Internal process to mutate character room pointers for when the character goes from room to room */
	UFUNCTION()
	void ProcessRoomChange(ASVSRoom* NewRoom);
#pragma region ="RoomTraversal"

#pragma region ="Combat"
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void HandlePrimaryAttack();
	
	/**
	 * Plays an Attack Animation
	 * @param TimerValue How much time it takes before another attack can execute.
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "SVS|Abilities|Combat")
	void NM_PlayAttackAnimation(const float TimerValue = 0.3f);

	/**
	 * Launches Character away from the provided FromLocation using the provided AttackForce.
	 * @param FromLocation Location of attacker or cause of launch
	 * @param InAttackForce Force applied to the launch
	*/
	UFUNCTION(BlueprintCallable)
	virtual void ApplyAttackImpactForce(const FVector FromLocation, const float InAttackForce) const override;
	/** Internal Multicast Method for Appy Attack Impract Force Interface Override */
	UFUNCTION(NetMulticast, Reliable)
	void NM_ApplyAttackImpactForce(const FVector FromLocation, const float InAttackForce) const;

	// TODO get values from weapon data asset
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Abilities")
	class USphereComponent* AttackZone;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|Abilities")
	UAnimMontage* AttackMontage;

	// TODO Rework this
	/** Tag name which specifies which characters can participate in combat */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Abilities|Combat", meta = (AllowPrivateAccess = "true"))
	FName CombatantTag = "Spy";
#pragma endregion="Combat"

#pragma region="InputCallMethods"
	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	/** Called for looking input */
	//void Look(const FInputActionValue& Value);
	/** Called for Interact Input */
	void RequestSprint(const FInputActionValue& Value);
	/** Called for Primary Attack Input */
	void RequestPrimaryAttack(const FInputActionValue& Value);
	/** Called for Next Trap Input */
	void RequestNextTrap(const FInputActionValue& Value);
	/** Called for Previous Trap Input */
	void RequestPrevTrap(const FInputActionValue& Value);
	/** Called for Interact Input */
	void RequestInteract(const FInputActionValue& Value);
#pragma endregion="InputCallMethods"

#pragma region="Ability System"
	/** Component of Playerstate */
	UPROPERTY()
	USpyAbilitySystemComponent* SpyAbilitySystemComponent;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	void AbilitySystemComponentInit();
	
	/** ASC Binding occurs in setupinput and onrep_playerstate due possibility of race condition
	  * bool prevents duplicate binding as it is called in both methods */
	bool bAbilitySystemComponentBound;
	void BindAbilitySystemComponentInput();

	/** Add initial abilities and effects */
	void AddStartupGameplayAbilities();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SVS|Abilities")
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SVS|Abilities")
	TArray<TSubclassOf<USpyGameplayAbility>> GameplayAbilities;

	friend USpyAttributeSet; // TODO 
	UPROPERTY()
	TObjectPtr<USpyAttributeSet> Attributes;

	/** GAS related tags */
	FGameplayTag SpyDeadTag;
	FGameplayTag EffectRemoveOnDeathTag;
#pragma endregion="Ability System"
	
};
