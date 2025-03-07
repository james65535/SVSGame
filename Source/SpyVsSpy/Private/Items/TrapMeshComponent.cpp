// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/TrapMeshComponent.h"

UTrapMeshComponent::UTrapMeshComponent()
{
}

void UTrapMeshComponent::BeginPlay()
{
	/** This component is just cosmetic */
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetSimulatePhysics(false);
	Super::BeginPlay();
}
