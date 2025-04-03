// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/UHeldItemMeshComponent.h"

UHeldItemMeshComponent::UHeldItemMeshComponent()
{
}

void UHeldItemMeshComponent::BeginPlay()
{
	/** This component is just cosmetic */
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetSimulatePhysics(false);
	Super::BeginPlay();
}
