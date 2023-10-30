// Fill out your copyright notice in the Description page of Project Settings.


#include "Rooms/SpyFurniture.h"

#include "SVSLogger.h"
#include "Components/ShapeComponent.h"
#include "Rooms/FurnitureInteractionComponent.h"
#include "Items/InventoryComponent.h"

ASpyFurniture::ASpyFurniture()
{
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("Inventory Component");
	
	FurnitureInteractionComponent = CreateDefaultSubobject<UFurnitureInteractionComponent>("Interaction Component");
	FurnitureInteractionComponent->SetIsReplicated(true);
}

void ASpyFurniture::BeginPlay()
{
	Super::BeginPlay();

	/** Collision Shape primarily used for Interactions */
	if (IsValid(CollisionShape))
	{
		CollisionShape->SetCollisionProfileName("Interact");
		CollisionShape->SetCollisionObjectType(ECC_GameTraceChannel1);
		CollisionShape->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);	
	}
	else
	{ UE_LOG(SVSLog, Warning, TEXT("Furniture: %s could not set collision profile for collision shape used for interactions"), *GetName()); }

	/** Add delegates for Overlaps */
	OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
	OnActorEndOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapEnd);
}

void ASpyFurniture::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Furniture: %s overlapped other actor: %s"), *GetName(), *OtherActor->GetName())
}

void ASpyFurniture::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Furniture: %s ended overlap with other actor: %s"), *GetName(), *OtherActor->GetName())
}
