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
	SharedParams.Condition = COND_AutonomousOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(USpyInteractionComponent, InteractableObjectInfo, SharedParams);
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
		// TODO Consider impact of handling multiple interactable actors
		SetLatestInteractableComponentFound(Component);
		return;
	}
}

void USpyInteractionComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsRunningClientOnly())
	{ return; }

	if (IsValid(LatestInteractableComponentFound.GetObjectRef()) &&
		LatestInteractableComponentFound->Execute_GetInteractableOwner(
			LatestInteractableComponentFound.GetObjectRef()) == OtherActor)
	{ SetLatestInteractableComponentFound(nullptr); }
	UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s no longer overlapping with actor: %s"), (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) ? *FString("Local") : *FString("Remote"), *GetOwner()->GetName(), *OtherActor->GetName());
}

bool USpyInteractionComponent::CanInteract() const
{
	return bCanInteractWithActor;
}

void USpyInteractionComponent::SetLatestInteractableComponentFound(UActorComponent* InFoundInteractableComponent)
{
	if (IsRunningClientOnly())
	{ return; }
	
	if (UKismetSystemLibrary::DoesImplementInterface(InFoundInteractableComponent, UInteractInterface::StaticClass()))
	{ LatestInteractableComponentFound = InFoundInteractableComponent; }
	else
	{ LatestInteractableComponentFound = nullptr; }

	if (IsValid(LatestInteractableComponentFound.GetObjectRef()))
	{ bCanInteractWithActor = true; }
	else
	{ bCanInteractWithActor = false; }
	
	InteractableObjectInfo = {LatestInteractableComponentFound, bCanInteractWithActor};
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, InteractableObjectInfo, this);
}

void USpyInteractionComponent::OnRep_InteractableInfo()
{
	if (IsValid(LatestInteractableComponentFound.GetObject()))
	{ LatestInteractableComponentFound->EnableInteractionVisualAid_Implementation(false);	}

	LatestInteractableComponentFound = InteractableObjectInfo.LatestInteractableComponentFound;
	bCanInteractWithActor = InteractableObjectInfo.bCanInteract;

	if (IsValid(InteractableObjectInfo.LatestInteractableComponentFound.GetObjectRef()))
	{
		LatestInteractableComponentFound->Execute_EnableInteractionVisualAid(
			LatestInteractableComponentFound.GetObjectRef(),
			true);
	}
}

TScriptInterface<IInteractInterface> USpyInteractionComponent::RequestInteractWithObject()
{
	if (!IsValid(LatestInteractableComponentFound.GetObjectRef()))
	{ return nullptr; }

	return LatestInteractableComponentFound;
}

void USpyInteractionComponent::S_RequestBasicInteractWithObject_Implementation()
{
	if (!IsValid(LatestInteractableComponentFound.GetObjectRef())) { return; }
	
	const bool bSuccessful = LatestInteractableComponentFound->Execute_Interact(LatestInteractableComponentFound.GetObjectRef(), GetOwner());
	UE_LOG(SVSLogDebug, Log, TEXT("Interaction success status: %s"), bSuccessful ? *FString("True") : *FString("False"));
}
