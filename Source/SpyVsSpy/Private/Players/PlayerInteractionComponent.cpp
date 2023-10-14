// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/PlayerInteractionComponent.h"

#include "SVSLogger.h"
#include "Players/InteractInterface.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UPlayerInteractionComponent::UPlayerInteractionComponent()
{
	SetGenerateOverlapEvents(true);
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	
	CapsuleRadius = 22.0f;
	CapsuleHalfHeight = 35.0f;
	/** Rotate Capsule so it across the area of both hands and not the entire body */
	SetWorldRotation(FRotator(90.0f, 0.0f, 90.0f));
}

void UPlayerInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;
	SharedParams.Condition = COND_OwnerOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(UPlayerInteractionComponent, LatestInteractableComponentFound, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UPlayerInteractionComponent, bCanInteractWithActor, SharedParams);
}

void UPlayerInteractionComponent::OnRep_LatestInteractableComponentFound()
{
	if (!IsValid(LatestInteractableComponentFound)) { return; }
	UE_LOG(SVSLogDebug, Log, TEXT("Pawn %s can interact with with component %s"), *GetOwner()->GetName(), *LatestInteractableComponentFound->GetName());
}

void UPlayerInteractionComponent::OnRep_bCanInteractWithActor()
{
	UE_LOG(SVSLogDebug, Log, TEXT("Pawn %s can interact: %s"), *GetOwner()->GetName(), bCanInteractWithActor ? *FString("True") : *FString("True")); 
}

void UPlayerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	/** Respond only to Custom Collision Channel 1: InteractChannel */
	SetCollisionProfileName("Interact");
	SetCollisionObjectType(ECC_GameTraceChannel1);
	SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);	
	
	OnComponentBeginOverlap.AddDynamic(this, &UPlayerInteractionComponent::OnOverlapBegin);
	OnComponentEndOverlap.AddDynamic(this, &UPlayerInteractionComponent::OnOverlapEnd);
}

void UPlayerInteractionComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (!IsValid(OtherActor) || GetOwnerRole() != ROLE_Authority) { return; }
	for (UActorComponent* Component : OtherActor->GetComponentsByInterface(UInteractInterface::StaticClass()))
	{
		bCanInteractWithActor = true;
		MARK_PROPERTY_DIRTY_FROM_NAME(UPlayerInteractionComponent, bCanInteractWithActor, this);
		LatestInteractableComponentFound = Component;
		MARK_PROPERTY_DIRTY_FROM_NAME(UPlayerInteractionComponent, LatestInteractableComponentFound, this);
		
		// TODO Consider impact of handling multiple interactable actors
		return;
	}
}

void UPlayerInteractionComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(LatestInteractableComponentFound)) { return; }
	
	// TODO Think about doing a validation check and also consider multiple actors
	bCanInteractWithActor = false;
	if (LatestInteractableComponentFound->GetOwner() == OtherActor)
	{
		LatestInteractableComponentFound = nullptr;
		MARK_PROPERTY_DIRTY_FROM_NAME(UPlayerInteractionComponent, LatestInteractableComponentFound, this);
	}
	UE_LOG(SVSLogDebug, Log, TEXT("Character: %s can no longer interact with actor: %s"), *GetOwner()->GetName(), *OtherActor->GetName());
}

void UPlayerInteractionComponent::RequestInteractWithObject() const
{
	S_RequestInteractWithObject();
}

void UPlayerInteractionComponent::S_RequestInteractWithObject_Implementation() const
{
	if (!IsValid(LatestInteractableComponentFound)) { return; }
	
	UE_LOG(SVSLogDebug, Log, TEXT("Server request interfact with %s:"), *LatestInteractableComponentFound->GetName());
	if (const IInteractInterface* InteractableComponent = Cast<IInteractInterface>(LatestInteractableComponentFound))
	{
		const bool bSuccessful = InteractableComponent->Execute_Interact(LatestInteractableComponentFound);
		UE_LOG(SVSLogDebug, Log, TEXT("Interaction success status: %s"), bSuccessful ? *FString("True") : *FString("False"));
	}
	C_RequestInteractWithObject();
}

void UPlayerInteractionComponent::C_RequestInteractWithObject_Implementation() const
{

}
