#include "WeaponBase.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "../Character/BaseCharacter.h"
#include "../PlayerController/BasePlayerController.h"
#include "Animation/AnimationAsset.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Casing.h"
#include "BATTLEFIELD/Components/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpWidget"));
	PickUpWidget->SetupAttachment(RootComponent);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeaponBase::OnSphereEndOverlap);

	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(false);
	}
	
	// if (!HasAuthority())
	// {
	// 	FireDelay = 0.001f;
	// }
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponBase, WeaponState);
	DOREPLIFETIME_CONDITION(AWeaponBase, bUseServerSideRewind,COND_OwnerOnly);
	// DOREPLIFETIME(AWeaponBase, Ammo);
}

void AWeaponBase::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		OwnerCharacter = nullptr;
		OwnerController = nullptr;
	}
	else
	{
		OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(Owner) : OwnerCharacter;
		if (OwnerCharacter && OwnerCharacter->GetEquippedWeapon() && OwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDAmmo();
		}
	}
}

void AWeaponBase::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                  const FHitResult& SweepResult)
{
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter)
	{
		if (WeaponType == EWeaponType::EWT_Flag && BaseCharacter->GetTeam() == Team)return;
		if (BaseCharacter->IsHoldingTheFlag())return;
		BaseCharacter->SetOverlappingWeapon(this);
	}
}

void AWeaponBase::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter)
	{
		if (WeaponType == EWeaponType::EWT_Flag && BaseCharacter->GetTeam() == Team)return;
		if (BaseCharacter->IsHoldingTheFlag())return;
		BaseCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeaponBase::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		++Sequence;
	}
}

void AWeaponBase::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority())return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeaponBase::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeaponBase::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority())return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter && OwnerCharacter->GetCombat() && IsFull())
	{
		OwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}

void AWeaponBase::SetHUDAmmo()
{
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter)
	{
		OwnerController = OwnerController == nullptr
			                  ? Cast<ABasePlayerController>(OwnerCharacter->Controller)
			                  : OwnerController;
		if (OwnerController)
		{
			OwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}
// void AWeaponBase::OnRep_Ammo()
// {
// 	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
// 	if (OwnerCharacter && OwnerCharacter->GetCombat() && IsFull())
// 	{
// 		OwnerCharacter->GetCombat()->JumpToShotgunEnd();
// 	}
// 	SetHUDAmmo();
// }

void AWeaponBase::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeaponBase::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
}

void AWeaponBase::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
	
}

void AWeaponBase::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeaponBase::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:\
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	}
}

void AWeaponBase::OnEquipped()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	EnableCustomDepth(false);
	
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr
							  ? Cast<ABasePlayerController>(OwnerCharacter->Controller)
							  : OwnerController;
		if (OwnerController && HasAuthority() && !OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.AddDynamic(this,&AWeaponBase::OnPingTooHigh);
		}
	}
}

void AWeaponBase::OnDropped()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
	
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr
							  ? Cast<ABasePlayerController>(OwnerCharacter->Controller)
							  : OwnerController;
		if (OwnerController && HasAuthority() && OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.RemoveDynamic(this,&AWeaponBase::OnPingTooHigh);
		}
	}
}

void AWeaponBase::OnEquippedSecondary()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	if (WeaponMesh)
	{
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
	}
	
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr
							  ? Cast<ABasePlayerController>(OwnerCharacter->Controller)
							  : OwnerController;
		if (OwnerController && HasAuthority() && OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.RemoveDynamic(this,&AWeaponBase::OnPingTooHigh);
		}
	}
	// EnableCustomDepth(true);
}

void AWeaponBase::ShowPickupWidget(bool bShowWidget)
{
	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(bShowWidget);
	}
}

void AWeaponBase::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	if (CasingClass)
	{
		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		const USkeletalMeshSocket* BulletShellSocket = WeaponMesh->GetSocketByName("BulletShellSpawnLocation");
		if (BulletShellSocket)
		{
			FTransform SocketTransform = BulletShellSocket->GetSocketTransform(WeaponMesh);
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(),
				                           SocketTransform.GetRotation().Rotator());
			}
		}
	}
	SpendRound();
}

void AWeaponBase::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
}

bool AWeaponBase::IsEmpty()
{
	return Ammo <= 0;
}

bool AWeaponBase::IsFull()
{
	return Ammo == MagCapacity;
}

FVector AWeaponBase::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlashSocket");
	if (MuzzleFlashSocket == nullptr)return FVector();
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;

	// DrawDebugSphere(GetWorld(),SphereCenter,SphereRadius,12,FColor::Red,true);
	// DrawDebugSphere(GetWorld(),EndLoc,4.f,12,FColor::Blue,true);
	// DrawDebugLine(GetWorld(),TraceStart,FVector(TraceStart + ToEndLoc * TRACE_LENGTH/ToEndLoc.Size()),FColor::Cyan,true);
	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}