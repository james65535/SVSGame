// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/InteractionComponent.h"
#include "Components/TimelineComponent.h"
#include "DoorInteractionComponent.generated.h"

class UAudioComponent;
class UTimelineComponent;

UDELEGATE(Category = "SVS|Door")
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorOpened);
UDELEGATE(Category = "SVS|Door")
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorClosed);

UENUM(BlueprintType)
enum class EDoorState
{
	Closed = 0 UMETA(DisplayName = "Closed"),
	Closing = 1 UMETA(DisplayName = "Closing"),
	Opened = 2 UMETA(DisplayName = "Opened"),
	Opening = 3 UMETA(DisplayName = "Opening"),
	Locked = 4 UMETA(DisplayName = "Locked"),
	Disabled = 5 UMETA(DisplayName = "Disabled"),
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPYVSSPY_API UDoorInteractionComponent : public UInteractionComponent
{
	GENERATED_BODY()

public:
	
	// Sets default values for this component's properties
	UDoorInteractionComponent();
	
	FOnDoorOpened OnDoorOpened;
	FOnDoorClosed OnDoorClosed;

	/** Timeline for door open / close */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UCurveFloat* DoorTransitionTimelineCurve;
	FName DoorTransitionTrackName = "DoorTransitionTrack";
	FName AppearTimelinePropertyName = "DoorTransitionAmount";

	/** Interact Interface Override */
	/** @return Success Status */
	virtual bool Interact_Implementation(AActor* InteractRequester) override;

	UFUNCTION(BlueprintCallable)
	bool IsOpen() const { return DoorState == EDoorState::Opened; }

	UFUNCTION(BlueprintCallable)
	EDoorState GetDoorState() const { return DoorState; }
	
	virtual void SetInteractionEnabled(const bool bIsEnabled) override;
	
private:

	// UTrapComponent* TrapComponent;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	EDoorState DoorState = EDoorState::Closed;
	
	/** Internal Methods for Door Opening / Closing */
	/** Initiate Door Opening Sequence */
	UFUNCTION(NetMulticast, Reliable)
	void NM_OpenDoor();
	/** Initiate Door Closing Sequence */
	UFUNCTION(NetMulticast, Reliable)
	void NM_CloseDoor();
	// TODO Add net multicasts
	/** Handle tasks upon door reaching Opened State */
	void DoorOpened();
	/** Handle tasks upon door reaching Closed State */
	void DoorClosed();
	/** Perform Door Opening/Closing Movements */
	void TransitionDoor(float DoorOpenedAmount);
	
	/** Timeline components for Opening / Closing Door */
	UPROPERTY(EditAnywhere)
	UTimelineComponent* DoorTransitionTimeline;
	FOnTimelineFloat OnDoorTransitionUpdate;
	FOnTimelineEvent OnDoorTransitionFinish;
	UFUNCTION()
	void DoorTransitionTimelineUpdate(float const OpenAmount);
	UFUNCTION()
	void DoorTransitionTimelineFinish();
	ETimelineDirection::Type TimelineDirection;
	
	/** Door Open Rotation Properties */
	FRotator StartRotation = FRotator::ZeroRotator;
	FRotator FinalRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere)
	float TimeToRotate = 1.0f;
	float CurrentRotationTime = 0.0f;
	UPROPERTY(EditAnywhere)
	FRuntimeFloatCurve OpenCurve;

	UPROPERTY(BlueprintReadWrite, EditAnywhere,  meta = (AllowPrivateAccess = "true"))
	UAudioComponent* DoorOpenSfx;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,  meta = (AllowPrivateAccess = "true"))
	UAudioComponent* DoorCloseSfx;

protected:

	virtual void BeginPlay() override;
	
};
