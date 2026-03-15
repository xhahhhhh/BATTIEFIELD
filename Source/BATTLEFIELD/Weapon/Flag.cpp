/**
 * @file Flag.cpp
 * @brief 旗帜实现
 */

#include "Flag.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

AFlag::AFlag()
{
	// 创建旗帜网格并设为根组件
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	SetRootComponent(FlagMesh);

	// 设置碰撞球体和拾取UI的附着点
	GetAreaSphere()->SetupAttachment(FlagMesh);
	GetPickUpWidget()->SetupAttachment(FlagMesh);

	// 初始禁用碰撞
	FlagMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFlag::Dropped()
{
	// 设置武器状态为掉落
	SetWeaponState(EWeaponState::EWS_Dropped);

	// 从持有者身上分离
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	FlagMesh->DetachFromComponent(DetachRules);

	// 清空所有者信息
	SetOwner(nullptr);
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
}

void AFlag::ResetFlag()
{
	// 清除持有者的旗帜状态
	ABaseCharacter* FlagBearer = Cast<ABaseCharacter>(GetOwner());
	if (FlagBearer)
	{
		FlagBearer->SetHoldingTheFlag(false);
		FlagBearer->SetOverlappingWeapon(nullptr);
	}

	// 仅服务器执行重置
	if (!HasAuthority()) return;

	// 重置武器状态
	SetWeaponState(EWeaponState::EWS_Initial);

	// 重新启用碰撞球体
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetAreaSphere()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	// 掉落并传送回初始位置
	Dropped();
	SetActorTransform(InitialTransform);
}

void AFlag::BeginPlay()
{
	Super::BeginPlay();

	// 记录初始变换
	InitialTransform = GetActorTransform();
}

void AFlag::OnEquipped()
{
	// 隐藏拾取提示
	ShowPickupWidget(false);

	// 禁用碰撞球体
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 禁用物理模拟
	FlagMesh->SetSimulatePhysics(false);
	FlagMesh->SetEnableGravity(false);

	// 设置仅查询碰撞，与动态世界物体重叠
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FlagMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	// 禁用轮廓高亮
	EnableCustomDepth(false);
}

void AFlag::OnDropped()
{
	// 服务器重新启用碰撞球体
	if (HasAuthority())
	{
		GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	// 启用物理模拟和重力
	FlagMesh->SetSimulatePhysics(true);
	FlagMesh->SetEnableGravity(true);

	// 启用完整碰撞
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

	// 忽略Pawn和Camera通道
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	// 启用蓝色轮廓高亮
	FlagMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	FlagMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
}
