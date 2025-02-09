// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Players/SpyCombatantInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "Items/InventoryBaseAsset.h"
#include "SpyCharacter.generated.h"

enum class EPlayerTeam : uint8;
class UPhysicsConstraintComponent;
class UInventoryWeaponAsset;
class UNiagaraSystem;
class USpyAbilitySystemComponent;
class ASpyPlayerState;
class ASVSRoom;
class UInventoryComponent;
class USpyInteractionComponent;
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

	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	ASpyPlayerState* GetSpyPlayerState() const;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	virtual void UpdateCameraLocation(const ASVSRoom* InRoom) const;
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	UInventoryComponent* GetPlayerInventoryComponent() const { return PlayerInventoryComponent; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	USpyInteractionComponent* GetInteractionComponent() const { return SpyInteractionComponent; }

	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	virtual void SetSpyHidden(const bool bIsSpyHidden);

	// TODO need to add a couple more reset actions here
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void ResetAttackHitFound() { bAttackHitFound = false; }
	
	// TODO refactor to reduce coupling
	/** Hack for getting attack hit location to effect */
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	// FVector_NetQuantize AttackHitLocation;

	FName GetWeaponHandSocket() const { return RightHandSocketName; };
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void StartPrimaryAttackWindow();
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void CompletePrimaryAttackWindow();
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void HandlePrimaryAttackHit(const FHitResult& HitResult);

	/**
	 * Plays an Attack Animation
	 * @param AttackMontage The Animation Montage to use for the attack
	 * @param TimerValue How much time it takes before another attack can execute.
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void PlayAttackAnimation(UAnimMontage* AttackMontage, const float TimerValue = 0.3f);
	
	/** Animation Montage for times of celebration such as winning match */
	UFUNCTION()
	bool PlayCelebrateMontage();

	/** Activities needed after finishing a match */
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	void DisableSpyCharacter();
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	UInventoryBaseAsset* GetEquippedItemAsset() const;

	
#pragma region="Team"
	// TODO refactor and move to playerstate using an enum
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	uint8 SpyTeam = 0;
	
	// TODO Refactor approach to materials and leverage a data asset
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UStaticMeshComponent* HatMeshComponent;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* TorsoTeamAMaterialInstance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* LegsTeamAMaterialInstance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* GlovesTeamAMaterialInstance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* CoatTeamAMaterialInstance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* HatTeamAMaterialInstance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* TorsoTeamBMaterialInstance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* LegsTeamBMaterialInstance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* GlovesTeamBMaterialInstance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* CoatTeamBMaterialInstance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Character")
	UMaterialInterface* HatTeamBMaterialInstance;
#pragma endregion="Team"
	
#pragma region="Health"
	UPROPERTY(BlueprintAssignable, Category = "SVS|Character")
	FCharacterDiedDelegate OnCharacterDied;
	virtual void RequestDeath();
	
	/** Can only be called by the Server. Removing on the Server will remove from Client too */
	virtual void RemoveCharacterAbilities();

	UFUNCTION(BlueprintCallable, Category = "SVS|Character|Attributes")
	virtual float GetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "SVS|Character|Attributes")
	virtual float GetMaxHealth() const;
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual bool IsAlive();
#pragma endregion="Health"

#pragma region="Movement"
	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	/** Called for looking input */
	//void Look(const FInputActionValue& Value);
	/** Called for Interact Input */
	void RequestSprint(const FInputActionValue& Value);
	/** Called for Primary Attack Input */
	void RequestPrimaryAttack(const FInputActionValue& Value);
	// TODO update these for refactor to inventorycomponent
	/** Called for Next Trap Input */
	void RequestEquipNextInventoryItem(const FInputActionValue& Value);
	/** Called for Previous Trap Input */
	void RequestEquipPreviousInventoryItem(const FInputActionValue& Value);
	/** Called for Interact Input */
	void RequestInteract();
#pragma endregion="Movement"

private:

	UPROPERTY()
	ASpyPlayerState* SpyPlayerState;

	void UpdateCharacterTeam(const EPlayerTeam InPlayerTeam);

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UIsoCameraComponent* FollowCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "SVS|Character")
	UInventoryComponent* PlayerInventoryComponent;

	UFUNCTION()
	void EquippedItemUpdated();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "SVS|Character")
	USpyInteractionComponent* SpyInteractionComponent;

#pragma region="CharacterVisibility"
	/** Controls whether the character is visible to other players */
	UPROPERTY()
	bool bIsHiddenInGame = true;
#pragma endregion="CharacterVisibility"
	
#pragma region="Movement"
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
#pragma endregion="Movement"
	
protected:

	UFUNCTION()
	void OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor);
	UFUNCTION()
	void OnOverlapEnd(class AActor* OverlappedActor, class AActor* OtherActor);
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS|Interact")
	void S_RequestInteract();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess), Category = "SVS|Animation")
	UAnimMontage* CelebrateMontage = nullptr;
	FOnMontageEnded CelebrateMontageEndedDelegate;
	UFUNCTION()
	void OnCelebrationMontageEnded(UAnimMontage* Montage, bool bInterrupted);

#pragma region="ClassOverrides"
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
#pragma endregion="ClassOverrides"
	
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
	/** Used to allow ProcessRoomChange to work when actor is moved by setactorlocation */
	bool bHasTeleported = false;
#pragma endregion ="RoomTraversal"

#pragma region ="Combat"
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS|Combat")
	void S_RequestPrimaryAttack();
	
	void SetAttackActive(const bool bEnabled) const;
	bool bAttackHitFound = false;
	// TODO remove after refactor
	// UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SVS|Abilities|Combat")
	// UNiagaraSystem* AttackImpactSpark; // TODO move back to ability system cue
	
	/**
	 * Plays an Attack Animation
	 * @param AttackMontage The Animation Montage to use for the attack
	 * @param TimerValue How much time it takes before another attack can execute.
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "SVS|Abilities|Combat")
	void NM_PlayAttackAnimation(UAnimMontage* AttackMontage, const float TimerValue = 0.3f);

	/**
	 * Launches Character away from the provided FromLocation using the provided AttackForce.
	 * @param FromLocation Location of attacker or causer of launch
	 * @param ImpactForce Force applied to the launch
	*/
	UFUNCTION(BlueprintCallable)
	virtual void ApplyAttackImpactForce_Implementation(const FVector FromLocation, const FVector ImpactForce) const override;
	/** Internal Multicast Method for Apply Attack Impact Force Interface Override */
	UFUNCTION(NetMulticast, Reliable)
	void NM_ApplyAttackImpactForce(const FVector FromLocation, const FVector InAttackForce) const;

	// TODO remove after refactor
	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Abilities|Combat", meta = (AllowPrivateAccess = "true"))
	// USkeletalMeshComponent* AttackZoneAlt;
	// TODO remove after finishing attack ability refactor
	// UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|Abilities")
	// UAnimMontage* AttackMontage;
	FOnMontageEnded AttackMontageEndedDelegate;
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// TODO Rework this
	/** Tag name which specifies which characters can participate in combat */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Abilities|Combat", meta = (AllowPrivateAccess = "true"))
	FName CombatantTag = FName("Spy");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	UInventoryWeaponAsset* CurrentHeldWeaponAsset;
	// TODO refactor much of this into inventory component and change this to equip item (weapon or trap)
	UFUNCTION(Server, Reliable, Category = "SVS|Character")
	void S_RequestEquipItem(const EItemRotationDirection InItemRotationDirection);

	/** Weapons attach to the right hand socket */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Abilities", meta = (AllowPrivateAccess = "true"))
	FName RightHandSocketName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Abilities", meta = (AllowPrivateAccess = "true"))
	/** Traps attach to the Left hand socket */
	FName LeftHandSocketName;
#pragma endregion="Combat"

#pragma region="CharacterDeath"
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS|Character")
	void S_RequestDeath();
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "SVS|Character")
	void NM_RequestDeath();
	void SetEnableDeathState(const bool bEnabled, const FVector& RespawnLocation = FVector::ZeroVector);
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "SVS|Character")
	void NM_SetEnableDeathState(const bool bEnabled, const FVector RespawnLocation = FVector::ZeroVector);

	FVector GetSpyRespawnLocation();
#pragma endregion="CharacterDeath"

#pragma region="Ability System"
	/** Ability System Component is owned by Playerstate */
	UPROPERTY()
	USpyAbilitySystemComponent* SpyAbilitySystemComponent;
	
	void AbilitySystemComponentInit();
	
	/** ASC Binding occurs in setupinput and onrep_playerstate due possibility of race condition
	  * bool prevents duplicate binding as it is called in both methods */
	bool bAbilitySystemComponentBound;
	void BindAbilitySystemComponentInput();

	/** Add initial abilities and effects */
	void AddStartupGameplayAbilities();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess), Category = "SVS|Abilities")
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess), Category = "SVS|Abilities")
	TArray<TSubclassOf<USpyGameplayAbility>> GameplayAbilities;

	friend USpyAttributeSet; // TODO 
	UPROPERTY()
	TObjectPtr<USpyAttributeSet> Attributes;

	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	virtual void FinishDeath();

	FTimerHandle FinishDeathTimerHandle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Character|Death")
	float FinishDeathDelaySeconds = 5.0f;
	
	/** GAS related tags */
	FGameplayTag SpyDeadTag;
	FGameplayTag EffectRemoveOnDeathTag;

	FGameplayAbilitySpecHandle TrapTriggerSpecHandle;
#pragma endregion="Ability System"
};
