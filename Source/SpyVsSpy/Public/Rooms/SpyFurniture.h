// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FurnitureBase.h"
#include "GameFramework/Actor.h"
#include "SpyFurniture.generated.h"

class UFurnitureInteractionComponent;
class UInventoryComponent;

UCLASS()
class SPYVSSPY_API ASpyFurniture : public AFurnitureBase
{
	GENERATED_BODY()
	
public:
	
	ASpyFurniture();

	UFUNCTION(BlueprintCallable, Category = "SVS|Furniture")
	UFurnitureInteractionComponent* GetInteractionComponent() const { return FurnitureInteractionComponent; }

	UFUNCTION(BlueprintCallable, Category = "SVS|Furniture")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	
protected:
	
	/** Class Overrides */
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (AllowPrivateAccess = "true"), Category = "SVS|Furniture")
	bool bInteractionEnabled = true;
	
	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor);

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, Meta = (AllowPrivateAccess = "true"), Category = "SVS|Furniture")
	UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "SVS|Character")
	UFurnitureInteractionComponent* FurnitureInteractionComponent;
};
