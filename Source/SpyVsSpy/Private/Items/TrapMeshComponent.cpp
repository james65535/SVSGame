// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/TrapMeshComponent.h"

UTrapMeshComponent::UTrapMeshComponent()
{
	AlwaysLoadOnClient = true;
	AlwaysLoadOnServer = false;
	bOwnerNoSee = false;
	SetIsReplicatedByDefault(false);
	bHiddenInGame = false;
}
