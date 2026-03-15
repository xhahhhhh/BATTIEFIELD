/**
 * @file Pickup.cpp
 * @brief 拾取物基类实现
 * 
 * 提供拾取物的通用功能实现，包括：
 * - 组件设置（碰撞球、网格、特效）
 * - 持续旋转动画
 * - 延迟重叠绑定
 * - 拾取音效和特效
 */

#include "Pickup.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "BATTLEFIELD/Weapon/WeaponTypes.h"

APickup::APickup()
{
	// 启用Tick更新用于旋转动画
	PrimaryActorTick.bCanEverTick = true;
	
	// 启用网络复制
	bReplicates = true;

	//============================
	// 创建根组件
	//============================
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	//============================
	// 创建球形碰撞组件
	//============================
	OverlapSphere = CreateDefaultSubobject<USphereComponent>("OverlapSphere");
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetSphereRadius(150.f);
	
	// 仅查询碰撞，不对物理做出响应
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	// 默认忽略所有通道
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	// 仅对Pawn通道响应重叠事件
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	// 向上偏移，使拾取物位于地面以上
	OverlapSphere->AddLocalOffset(FVector(0.0f, 0.0f, 40.0f));

	//============================
	// 创建拾取物网格
	//============================
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>("PickupMesh");
	PickupMesh->SetupAttachment(OverlapSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetRelativeScale3D(FVector(5.f, 5.f, 5.f));
	
	// 启用自定义深度，用于轮廓高亮效果
	PickupMesh->SetRenderCustomDepth(true);
	PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);

	//============================
	// 创建持续特效组件
	//============================
	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>("PickupEffectComponent");
	PickupEffectComponent->SetupAttachment(RootComponent);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();
	
	// 仅在服务器上设置延迟绑定
	// 防止生成时立即与生成它的玩家重叠触发
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			BindOverlapTimer,
			this,
			&APickup::BindOverlapTimerFinished,
			BindOverlapTime
		);
	}
}

void APickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                              const FHitResult& SweepResult)
{
	// 基类不实现具体逻辑，由子类重写
	// 子类应在此检查重叠的是否是玩家，并应用相应效果
}

void APickup::BindOverlapTimerFinished()
{
	// 延迟绑定重叠事件，防止生成时立即触发
	OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereOverlap);
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 持续旋转拾取物网格，创建浮动效果
	if (PickupMesh)
	{
		PickupMesh->AddWorldRotation(FRotator(0.0f, BaseTurnRate * DeltaTime, 0.0f));
	}
}

void APickup::Destroyed()
{
	Super::Destroyed();

	//============================
	// 播放拾取音效
	//============================
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	//============================
	// 生成拾取特效
	//============================
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
}
