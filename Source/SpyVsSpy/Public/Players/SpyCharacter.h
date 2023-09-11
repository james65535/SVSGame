// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SpyCharacter.generated.h"

class ASVSRoom;
class UIsoCameraComponent;
class UPlayerInventoryComponent;
class UPlayerInteractionComponent;

UCLASS()
class SPYVSSPY_API ASpyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	
	ASpyCharacter();

	/** Returns CameraBoom subobject **/
	// FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	// /** Returns FollowCamera subobject **/
	// FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable, Category = "SVS Character")
	void UpdateCameraLocation(const ASVSRoom* InRoom) const;
	
	UFUNCTION(BlueprintCallable, Category = "SVS Character")
	UPlayerInventoryComponent* GetPlayerInventoryComponent() const { return PlayerInventoryComponent; };
	UFUNCTION(BlueprintCallable, Category = "SVS Character")
	UPlayerInteractionComponent* GetPlayerInteractionComponent() const { return PlayerInteractionComponent; };
	// TODO Implement
	// UFUNCTION(BlueprintCallable, Category = "SVS Character")
	//UPlayerHealthComponent* GetPlayerHealthComponent() const { return PlayerHealthComponent; }
	
private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "SVS Character")
	UPlayerInventoryComponent* PlayerInventoryComponent;
	// TODO Implement
	// UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), EditInstanceOnly, Category = "SVS Character")
	// UPlayerHealthComponent* PlayerHealthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "SVS Character")
	UPlayerInteractionComponent* PlayerInteractionComponent;
	
	/** Camera boom positioning the camera behind the character */
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	// class USpringArmComponent* CameraBoom;
	// /** Follow camera */
	 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	 class UIsoCameraComponent* FollowCamera;
	

	
#pragma region="MovementControls"
	/** Movement Controls */
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;
	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;
	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;
#pragma endregion="MovementControls"
	
protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	//void Look(const FInputActionValue& Value);

	/** Called for Interact Input */
	void Interact(const FInputActionValue& Value);
	
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;
	
};

