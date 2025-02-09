// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon.h"

#include "SVSLogger.h"
#include "Items/InventoryWeaponAsset.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Players/SpyCharacter.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
	if (IsValid(SkeletalMeshComponent))
	{
		RootComponent = SkeletalMeshComponent;
		SkeletalMeshComponent->AlwaysLoadOnClient = true;
		SkeletalMeshComponent->AlwaysLoadOnServer = true;
		SkeletalMeshComponent->bOwnerNoSee = false;
		SkeletalMeshComponent->SetIsReplicated(true);
		
		/** This actor will be attached to a Character actor and
		 * UE does not allow a child actor to simulate physics differently than the root component,
		 * in this case the character capsule. For collision hits, we must rely on shape casts for
		 * hit events instead of physics simulation
		 */

		/** Engine Collision Profile sets object type to ECC_GameTraceChannel2 */
		SkeletalMeshComponent->SetCollisionProfileName("CombatantPreset");
		SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::ProbeOnly);
		SkeletalMeshComponent->SetSimulatePhysics(false);
		SkeletalMeshComponent->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;
		SkeletalMeshComponent->SetEnableGravity(false);
		SkeletalMeshComponent->SetNotifyRigidBodyCollision(true);
		SkeletalMeshComponent->SetGenerateOverlapEvents(false);
		SkeletalMeshComponent->CanCharacterStepUpOn = ECB_No;
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;
	SharedParams.Condition = COND_None;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, WeaponMesh, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, bEnableOnTickComponentSweeps, SharedParams);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableOnTickComponentSweeps)
	{ SweepWeapon(); }
}

void AWeapon::SweepWeapon()
{
	if (UWorld* WeaponWorld = GetWorld())
	{
		/** Calc distances */
		const FVector TraceStart = GetMesh()->GetComponentLocation();
		const FVector Delta = TraceStart - OnComponentSweepEnableStartLocation;
		const FVector TraceEnd = TraceStart + Delta;
		/** Update Start Location for next tick */
		OnComponentSweepEnableStartLocation = TraceEnd;
			
		/** Recalc here to account for precision loss of float addition */
		float DeltaSizeSq = (TraceEnd - TraceStart).SizeSquared();
		const FQuat InitialRotationQuat = GetMesh()->GetComponentTransform().GetRotation();

		/** ComponentSweepMulti does nothing if moving < KINDA_SMALL_NUMBER in distance, so
		 * it's important to not try to sweep distances smaller than that. */ 
		constexpr float MinMovementDistSq = FMath::Square(4.f* UE_KINDA_SMALL_NUMBER);
		if (DeltaSizeSq > MinMovementDistSq)
		{
			TArray<FHitResult> OutHits;
			const bool bOutHitsFound = WeaponWorld->ComponentSweepMulti(
				OutHits,
				GetMesh(),
				TraceStart,
				TraceEnd,
				InitialRotationQuat,
				SweepQueryParams);

			for (FHitResult OutHit : OutHits)
			{
				if (GetAttachParentActor() != OutHit.Component->GetAttachParentActor())
				{
					if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetAttachParentActor()))
					{ SpyCharacter->HandlePrimaryAttackHit(OutHit); }
				}
			}
		}
	}
}

void AWeapon::OnRep_bEnableOnTickComponentSweeps()
{
	if (bEnableOnTickComponentSweeps)
	{ OnComponentSweepEnableStartLocation = GetMesh()->GetComponentLocation(); } else
	{ OnComponentSweepEnableStartLocation = FVector::ZeroVector; }
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AWeapon::OnRep_SetMesh()
{
	GetMesh()->SetSkeletalMesh(WeaponMesh);
	
	/* Determine if initial visibility needs to be hidden for remote players */
	if (const AActor* OwningActor = GetAttachParentActor())
	{ SetActorHiddenInGame(OwningActor->IsHidden()); }
}

bool AWeapon::LoadWeaponPropertyValuesFromDataAsset(const UInventoryWeaponAsset* InventoryWeaponAsset)
{
	if (!IsValid(InventoryWeaponAsset))
	{ return false; }

	WeaponMesh = InventoryWeaponAsset->WeaponMesh;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, WeaponMesh, this);
	GetMesh()->SetSkeletalMesh(WeaponMesh);
	
	WeaponType = InventoryWeaponAsset->WeaponType;

	/** Damage Info */
	bInstaKillEnabled = InventoryWeaponAsset->bInstaKillEnabled;
	if (!bInstaKillEnabled)
	{ WeaponAttackBaseDamage = InventoryWeaponAsset->WeaponAttackBaseDamage; }

	/** VFX */
	AttackVisualEffect = InventoryWeaponAsset->AttackVisualEffect;
	AttackDamageVisualEffect = InventoryWeaponAsset->AttackDamageVisualEffect;
	AttackFatalDamageVisualEffect = InventoryWeaponAsset->AttackFatalDamageVisualEffect;

	/** SFX */
	AttackSoundEffect = InventoryWeaponAsset->AttackSoundEffect;
	AttackDamageSoundEffect = InventoryWeaponAsset->AttackDamageSoundEffect;

	return true;
}

void AWeapon::EnableOnTickComponentSweeps(const bool bEnable)
{
	bEnableOnTickComponentSweeps = bEnable;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bEnableOnTickComponentSweeps, this);
	
	/** Perform onrep function on server as well */
	if (!IsRunningClientOnly())
	{ OnRep_bEnableOnTickComponentSweeps();}
}

void AWeapon::UpdateCollisionChannelResponseToBlock(const ECollisionChannel EnemyObjectChannel, const ECollisionChannel SelfObjectChannel)
{
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::ProbeOnly);
	SkeletalMeshComponent->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;
	SkeletalMeshComponent->SetCollisionResponseToChannel(EnemyObjectChannel,ECR_Block);
	SkeletalMeshComponent->SetCollisionResponseToChannel(SelfObjectChannel,ECR_Ignore);
}

