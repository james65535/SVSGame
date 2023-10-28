// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Items/InventoryWeaponAsset.h"
#include "Players/SpyCombatantInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "Items/InteractInterface.h"
#include "SpyCharacter.generated.h"

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

/** Enum to track preference for requesting previous or next item */
UENUM(BlueprintType)
enum class EItemRotationDirection : uint8
{
	Previous						UMETA(DisplayName = "Previous Item"),
	Next						UMETA(DisplayName = "NextItem")
};

UCLASS()
class SPYVSSPY_API ASpyCharacter : public ACharacter, public IAbilitySystemInterface, public ISpyCombatantInterface
{
	GENERATED_BODY()

public:
	
	ASpyCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	
	/** Animation Montage for times of celebration such as winning match */
	UFUNCTION()
	bool PlayCelebrateMontage();

	/** Activities needed after finishing a match */
	UFUNCTION(NetMulticast, Reliable)
	void NM_FinishedMatch();

	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void SetWeaponMesh(UStaticMesh* InMesh);

	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	UInventoryWeaponAsset* GetHeldWeapon() const { return CurrentHeldWeapon; }

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
	/** Called for Next Trap Input */
	void RequestNextTrap(const FInputActionValue& Value);
	/** Called for Previous Trap Input */
	void RequestPreviousTrap(const FInputActionValue& Value);
	/** Called for Interact Input */
	void RequestInteract();
	/** Called for Trap Trigger */
	void RequestTrapTrigger();
#pragma endregion="Movement"

private:

	UPROPERTY()
	ASpyPlayerState* SpyPlayerState;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UIsoCameraComponent* FollowCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "SVS|Character")
	UInventoryComponent* PlayerInventoryComponent;
	
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

	/** Attack Overlaps */
	UFUNCTION()
	void OnAttackComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnAttackComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS|Interact")
	void S_RequestInteract(UObject* InInteractableActor);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess), Category = "SVS|Animation")
	UAnimMontage* CelebrateMontage = nullptr;
	FOnMontageEnded CelebrateMontageEndedDelegate;
	UFUNCTION()
	void OnCelebrationMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
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
#pragma endregion ="RoomTraversal"

#pragma region ="Combat"
	void SetEnabledAttackState(const bool bEnabled) const;
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void HandlePrimaryAttackAbility(AActor* OverlappedSpyCombatant);
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void HandlePrimaryAttackOverlap(AActor* OverlappedSpyCombatant);
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SVS|Abilities|Combat")
	UNiagaraSystem* AttackImpactSpark; // TODO move back to ability system cue
	bool bAttackHitFound = false;
	// TODO review if there should be a server only call
	/**
	 * Plays an Attack Animation
	 * @param TimerValue How much time it takes before another attack can execute.
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void PlayAttackAnimation(const float TimerValue = 0.3f);
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "SVS|Abilities|Combat")
	void NM_PlayAttackAnimation(const float TimerValue = 0.3f);

	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities|Combat")
	void HandleTrapTrigger();

	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "SVS|Abilities|Combat")
	void C_RequestTrapTrigger();
	/**
	 * Plays an Attack Animation
	 * @param TimerValue How much time it takes before another attack can execute.
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "SVS|Abilities|Combat")
	void NM_PlayTrapTriggerAnimation(const float TimerValue = 0.3f);

	/**
	 * Launches Character away from the provided FromLocation using the provided AttackForce.
	 * @param FromLocation Location of attacker or cause of launch
	 * @param InAttackForce Force applied to the launch
	*/
	UFUNCTION(BlueprintCallable)
	virtual void ApplyAttackImpactForce(const FVector FromLocation, const FVector InAttackForce) const override;
	/** Internal Multicast Method for Appy Attack Impact Force Interface Override */
	UFUNCTION(NetMulticast, Reliable)
	void NM_ApplyAttackImpactForce(const FVector FromLocation, const FVector InAttackForce) const;

	// TODO get values from weapon data asset
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS|Abilities")
	class USphereComponent* AttackZone;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|Abilities")
	UAnimMontage* AttackMontage;
	FOnMontageEnded AttackMontageEndedDelegate;
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// TODO Rework this
	/** Tag name which specifies which characters can participate in combat */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Abilities|Combat", meta = (AllowPrivateAccess = "true"))
	FName CombatantTag = FName("Spy");
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Character|Combat")
	UStaticMeshComponent* WeaponMeshComponent;
	// TODO move to inventory component
	UPROPERTY(ReplicatedUsing = OnRep_ActiveWeaponInventoryIndex)
	int ActiveWeaponInventoryIndex = 0;
	UFUNCTION()
	void OnRep_ActiveWeaponInventoryIndex();
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (AllowPrivateAccess), Category = "SVS|Inventory")
	UInventoryWeaponAsset* CurrentHeldWeapon;
	UFUNCTION(Server, Reliable, Category = "SVS|Character")
	void S_RequestEquipWeapon(const EItemRotationDirection InItemRotationDirection);
#pragma endregion="Combat"

#pragma region="CharacterDeath"
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS|Character")
	void S_RequestDeath();
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "SVS|Character")
	void NM_RequestDeath();
	void SetEnableDeathState(const bool bEnabled);
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
