// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "SpyInteractionComponent.generated.h"

class IInteractInterface;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SPYVSSPY_API USpyInteractionComponent : public UCapsuleComponent
{
	GENERATED_BODY()

public:

	USpyInteractionComponent();

	
	/** @return The last interactable object to have overlapped this component */
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	TScriptInterface<IInteractInterface> GetLatestInteractableComponent() { return LatestInteractableComponentFound; }
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	void RequestInteractWithObject();
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS|Character")
	void S_RequestInteractWithObject();
	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "SVS|Character")
	void C_RequestInteractWithObject();

	UFUNCTION(BlueprintCallable, Category = "SVS|Character")
	bool CanInteractWithKnownInteractionInterface() const;

private:
	
	/** Most recently found overlapping component which satisfies interact interface */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (AllowPrivateAccess="true"), ReplicatedUsing=OnRep_LatestInteractableComponentFound, Category = "SVS|Character")
	TScriptInterface<IInteractInterface> LatestInteractableComponentFound;
	UFUNCTION()
	void OnRep_LatestInteractableComponentFound();
	UFUNCTION()
	void SetLatestInteractableComponentFound(UActorComponent* InFoundInteractableComponent);
	
protected:

	/** Class Overrides */
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_bCanInteractWithActor, VisibleAnywhere, Category = "SVS|Character")
	bool bCanInteractWithActor = false;
	UFUNCTION()
	void OnRep_bCanInteractWithActor();
	
};
