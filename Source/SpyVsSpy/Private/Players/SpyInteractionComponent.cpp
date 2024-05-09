// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyInteractionComponent.h"

#include "SVSLogger.h"
#include "Items/InteractInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

USpyInteractionComponent::USpyInteractionComponent()
{
	SetGenerateOverlapEvents(true);
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	
	CapsuleRadius = 22.0f;
	CapsuleHalfHeight = 50.0f;
	/** Rotate Capsule so it across the area of both hands and not the entire body */
	SetWorldRotation(FRotator(90.0f, 0.0f, 90.0f));
}

void USpyInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;
	SharedParams.Condition = COND_OwnerOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(USpyInteractionComponent, LatestInteractableComponentFound, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(USpyInteractionComponent, bCanInteractWithActor, SharedParams);
}

void USpyInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	/** Respond only to Custom Collision Channel 1: InteractChannel */
	SetCollisionProfileName("Interact");
	SetCollisionObjectType(ECC_GameTraceChannel1);
	SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);	
	
	OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);
	OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnOverlapEnd);
}

void USpyInteractionComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (!IsValid(OtherActor) || GetOwnerRole() != ROLE_Authority)
	{ return; }
	
	for (UActorComponent* Component : OtherActor->GetComponentsByInterface(UInteractInterface::StaticClass()))
	{
		UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s overlapped with interactable actor: %s"), (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) ? *FString("Local") : *FString("Remote"), *GetOwner()->GetName(), *OtherActor->GetName());
		SetLatestInteractableComponentFound(Component);
		
		// TODO Consider impact of handling multiple interactable actors
		return;
	}
}

void USpyInteractionComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (GetOwnerRole() != ROLE_Authority || !IsValid(LatestInteractableComponentFound.GetObjectRef()))
	{ return; }

	if (LatestInteractableComponentFound->Execute_GetInteractableOwner(LatestInteractableComponentFound.GetObjectRef()) == OtherActor)
	{
		UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s ended overlap with interactable actor: %s"), (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) ? *FString("Local") : *FString("Remote"), *GetOwner()->GetName(), *OtherActor->GetName());
		SetLatestInteractableComponentFound(nullptr);
	}
	UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s no longer overlapping with actor: %s"), (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) ? *FString("Local") : *FString("Remote"), *GetOwner()->GetName(), *OtherActor->GetName());
}

bool USpyInteractionComponent::CanInteract() const
{
	return bCanInteractWithActor;
}

void USpyInteractionComponent::OnRep_LatestInteractableComponentFound()
{
	if (!IsValid(LatestInteractableComponentFound.GetObjectRef())) { return; }
	UE_LOG(SVSLogDebug, Log, TEXT("Pawn %s can interact with with component %s"), *GetOwner()->GetName(), *LatestInteractableComponentFound.GetObjectRef()->GetName());
}

void USpyInteractionComponent::SetLatestInteractableComponentFound(UActorComponent* InFoundInteractableComponent)
{
	if (GetOwnerRole() != ROLE_Authority)
	{ return; }
	
	if (UKismetSystemLibrary::DoesImplementInterface(InFoundInteractableComponent, UInteractInterface::StaticClass()))
	{ LatestInteractableComponentFound = InFoundInteractableComponent; }
	else
	{ LatestInteractableComponentFound = nullptr; }
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LatestInteractableComponentFound, this)

	if (IsValid(LatestInteractableComponentFound.GetObjectRef()))
	{ bCanInteractWithActor = true; }
	else
	{ bCanInteractWithActor = false; }
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bCanInteractWithActor, this);
}

void USpyInteractionComponent::OnRep_bCanInteractWithActor()
{
	UE_LOG(SVSLogDebug, Log, TEXT("Pawn %s can interact: %s"), *GetOwner()->GetName(), bCanInteractWithActor ? *FString("True") : *FString("True")); 
}

TScriptInterface<IInteractInterface> USpyInteractionComponent::RequestInteractWithObject()
{
	UE_LOG(SVSLogDebug, Log, TEXT("Character Triggered Interact"));
	if (!IsValid(LatestInteractableComponentFound.GetObjectRef())) { return nullptr; }

	return LatestInteractableComponentFound;
}

void USpyInteractionComponent::S_RequestBasicInteractWithObject_Implementation()
{
	if (!IsValid(LatestInteractableComponentFound.GetObjectRef())) { return; }
	
	const bool bSuccessful = LatestInteractableComponentFound->Execute_Interact(LatestInteractableComponentFound.GetObjectRef(), GetOwner());
	UE_LOG(SVSLogDebug, Log, TEXT("Interaction success status: %s"), bSuccessful ? *FString("True") : *FString("False"));
}
