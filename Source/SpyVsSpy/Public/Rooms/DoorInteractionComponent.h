// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Players/InteractInterface.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "DoorInteractionComponent.generated.h"

class UAudioComponent;
class UTimelineComponent;

UDELEGATE(Category = "SVS Door")
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorOpened);
UDELEGATE(Category = "SVS Door")
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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPYVSSPY_API UDoorInteractionComponent : public UActorComponent, public IInteractInterface
{
	GENERATED_BODY()

public:
	
	// Sets default values for this component's properties
	UDoorInteractionComponent();
	
	FOnDoorOpened OnDoorOpened;
	FOnDoorClosed OnDoorClosed;

	/** Timeline for door open / close */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SVS Door")
	UCurveFloat* DoorTransitionTimelineCurve;
	FName DoorTransitionTrackName = "DoorTransitionTrack";
	FName AppearTimelinePropertyName = "DoorTransitionAmount";

	/** Interact Interface Override */
	/** @return Success Status */
	virtual bool Interact_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "SVS Door")
	bool IsOpen() const { return DoorState == EDoorState::Opened; }

	UFUNCTION(BlueprintCallable, Category = "SVS Door")
	EDoorState GetDoorState() const { return DoorState; }

	UFUNCTION(BlueprintCallable, Category = "SVS Door")
	void SetInteractionEnabled(const bool bIsEnabled);
	
private:

	// UTrapComponent* TrapComponent;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess = "true"), Category = "SVS Door")
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
	UPROPERTY(EditAnywhere, Category = "SVS Door")
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
	FRotator FinalRotation = FRotator(0.0f, 90.0f, 0.0f);
	UPROPERTY(EditAnywhere)
	float TimeToRotate = 1.0f;
	float CurrentRotationTime = 0.0f;
	UPROPERTY(EditAnywhere, Category = "SVS Door")
	FRuntimeFloatCurve OpenCurve;

	UPROPERTY(BlueprintReadWrite, EditAnywhere,  meta = (AllowPrivateAccess = "true"), Category = "SVS Door")
	UAudioComponent* DoorOpenSfx;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,  meta = (AllowPrivateAccess = "true"), Category = "SVS Door")
	UAudioComponent* DoorCloseSfx;

protected:

	virtual void BeginPlay() override;
};
