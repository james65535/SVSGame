// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "PlayerInteractionComponent.generated.h"

class IInteractInterface;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SPYVSSPY_API UPlayerInteractionComponent : public UCapsuleComponent
{
	GENERATED_BODY()

public:

	UPlayerInteractionComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps);

	/*
	 * Uses overlaps with capsule to determine if player can interact with something.
	 * Animations will use this state to adjust the player to make them look furtive.
	 * The player will need to issue an interact request to follow through interacting
	 * with the overlapped object
	 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Current Interaction State */
	UPROPERTY(BlueprintReadOnly, Replicated, VisibleAnywhere, Category = "SVS Character")
	bool bCanInteractWithActor = false;
	/** @return The last interactable object to have overlapped this component */
	UFUNCTION(BlueprintCallable, Category = "SVS Character")
	UActorComponent* GetLatestInteractableComponent() const { return LatestInteractableComponentFound; }
	
	UFUNCTION(BlueprintCallable, Category = "SVS Character")
	void RequestInteractWithObject() const;
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS Character")
	void S_RequestInteractWithObject() const;
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "SVS Character")
	void NM_RequestInteractWithObject() const;

private:
	
	/** Most recently found overlapping component which satisfies interact interface */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (AllowPrivateAccess="true"), Replicated, Category = "SVS Character")
	UActorComponent* LatestInteractableComponentFound;
	
protected:

	virtual void BeginPlay() override;

	
};