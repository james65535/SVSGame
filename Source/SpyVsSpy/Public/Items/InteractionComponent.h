// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/InteractInterface.h"
#include "InteractionComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPYVSSPY_API UInteractionComponent : public UActorComponent, public IInteractInterface
{
	GENERATED_BODY()

public:	
	
	UInteractionComponent();
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Interaction")
	virtual void SetInteractionEnabled(const bool bIsEnabled);
	UFUNCTION(BlueprintCallable, Category = "SVS|Interaction")
	virtual bool IsInteractionEnabled() const;

protected:

	/** Interact Interface Override */
	/**
	 * @brief Request Interaction
	 * @param InteractRequester The Actor who initiated this Interact Request
	 * @return Interaction Success Status
	 */
	virtual bool Interact_Implementation(AActor* InteractRequester) override;
	/**
	 * @brief Provide a listing of inventory items to interaction requester
	 * @param RequestedPrimaryAssetIds An array to populate with Primary Asset Ids of inventory items found
	 * @param RequestedPrimaryAssetType The type of asset to get
	 */
	virtual void GetInventoryListing_Implementation(TArray<FPrimaryAssetId>& RequestedPrimaryAssetIds, const FPrimaryAssetType RequestedPrimaryAssetType) override;

	virtual AActor* GetInteractableOwner_Implementation() override;

	virtual bool CheckHasTrap_Implementation() override;
	virtual bool HasInventory_Implementation() override;

private:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (AllowPrivateAccess = "true"), Category = "SVS|Furniture")
	bool bInteractionEnabled = true;
		
};
