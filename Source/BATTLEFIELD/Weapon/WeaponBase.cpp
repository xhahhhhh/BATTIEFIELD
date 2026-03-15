/**
 * @file WeaponBase.cpp
 * @brief 武器基类实现
 * 
 * 实现武器的基础功能：组件创建、状态管理、弹药同步、拾取检测等
 */

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

	//============================
	// 创建武器网格
	//============================
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	// 默认禁用碰撞，根据状态启用
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 启用自定义深度高亮（蓝色）
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	//============================
	// 创建拾取范围球体
	//============================
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//============================
	// 创建拾取提示Widget
	//============================
	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpWidget"));
	PickUpWidget->SetupAttachment(RootComponent);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// 在游戏开始时启用拾取检测
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeaponBase::OnSphereEndOverlap);

	// 初始隐藏拾取提示
	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(false);
	}
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 配置网络复制属性
	DOREPLIFETIME(AWeaponBase, WeaponState);
	// 仅对所有者复制倒带设置
	DOREPLIFETIME_CONDITION(AWeaponBase, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeaponBase::OnRep_Owner()
{
	Super::OnRep_Owner();
	
	// 所有者变更时更新引用
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
	// 检测玩家进入拾取范围
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter)
	{
		// 旗帜不能拾取己方旗帜
		if (WeaponType == EWeaponType::EWT_Flag && BaseCharacter->GetTeam() == Team) return;
		// 已持有旗帜时不能拾取其他武器
		if (BaseCharacter->IsHoldingTheFlag()) return;
		
		BaseCharacter->SetOverlappingWeapon(this);
	}
}

void AWeaponBase::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// 检测玩家离开拾取范围
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter)
	{
		if (WeaponType == EWeaponType::EWT_Flag && BaseCharacter->GetTeam() == Team) return;
		if (BaseCharacter->IsHoldingTheFlag()) return;
		
		BaseCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeaponBase::SpendRound()
{
	// 消耗一发弹药
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
	
	if (HasAuthority())
	{
		// 服务器通知客户端同步
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		// 客户端增加待处理计数
		++Sequence;
	}
}

void AWeaponBase::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	// 服务器不处理
	if (HasAuthority()) return;
	
	// 同步服务器弹药量，考虑待处理的本地预测
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeaponBase::AddAmmo(int32 AmmoToAdd)
{
	// 增加弹药（服务器执行）
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeaponBase::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	// 服务器不处理
	if (HasAuthority()) return;
	
	// 客户端增加弹药
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	
	// 霰弹枪满弹时跳过后坐力动画
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter && OwnerCharacter->GetCombat() && IsFull())
	{
		OwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}

void AWeaponBase::SetHUDAmmo()
{
	// 更新HUD弹药显示
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

void AWeaponBase::EnableCustomDepth(bool bEnable)
{
	// 设置自定义深度（用于丢弃时的高亮效果）
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
	// 延迟过高时禁用服务器倒带
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeaponBase::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeaponBase::OnWeaponStateSet()
{
	// 根据武器状态执行相应处理
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	}
}

void AWeaponBase::OnEquipped()
{
	// 装备状态：隐藏拾取提示、禁用碰撞、关闭物理
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// SMG特殊处理（需要物理碰撞）
	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	
	EnableCustomDepth(false);

	// 绑定高延迟检测委托
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr
			                  ? Cast<ABasePlayerController>(OwnerCharacter->Controller)
			                  : OwnerController;
		if (OwnerController && HasAuthority() && !OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.AddDynamic(this, &AWeaponBase::OnPingTooHigh);
		}
	}
}

void AWeaponBase::OnDropped()
{
	// 丢弃状态：启用物理、设置碰撞、高亮显示
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

	// 解绑高延迟检测委托
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr
			                  ? Cast<ABasePlayerController>(OwnerCharacter->Controller)
			                  : OwnerController;
		if (OwnerController && HasAuthority() && OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.RemoveDynamic(this, &AWeaponBase::OnPingTooHigh);
		}
	}
}

void AWeaponBase::OnEquippedSecondary()
{
	// 副武器状态：类似装备状态但保持高亮
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

	// 解绑高延迟检测委托
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr
			                  ? Cast<ABasePlayerController>(OwnerCharacter->Controller)
			                  : OwnerController;
		if (OwnerController && HasAuthority() && OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.RemoveDynamic(this, &AWeaponBase::OnPingTooHigh);
		}
	}
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
	// 播放射击动画
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	
	// 生成弹壳
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
	
	// 消耗弹药
	SpendRound();
}

void AWeaponBase::Dropped()
{
	// 丢弃武器处理
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
	// 计算带散射的射线终点
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlashSocket");
	if (MuzzleFlashSocket == nullptr) return FVector();
	
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}
