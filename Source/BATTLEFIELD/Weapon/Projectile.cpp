/**
 * @file Projectile.cpp
 * @brief 投射物基类实现
 * 
 * 实现投射物的通用功能，包括碰撞处理、特效播放、范围伤害计算等
 */

#include "Projectile.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "../Character/BaseCharacter.h"
#include "../BATTLEFIELD.h"
#include "NiagaraComponent.h"

AProjectile::AProjectile()
{
	// 启用每帧Tick，启用网络复制
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// 创建碰撞盒作为根组件
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);

	// 设置碰撞属性
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);

	// 投射物移动组件通常在子类中创建
	// ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	// ProjectileMovementComponent->bRotationFollowsVelocity = true;

	// 仅在服务器上绑定碰撞事件
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 生成曳光效果
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime
	);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	// 命中时销毁投射物
	// 子类可重写此函数实现特殊命中逻辑
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	// 播放命中特效
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	// 播放命中音效
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void AProjectile::ExplodeDamage()
{
	// 获取发射者
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			// 应用带衰减的范围伤害
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,                       // 伤害源
				Damage,                     // 基础伤害
				10.f,                       // 最小伤害
				GetActorLocation(),         // 爆炸中心
				DamageInnerRadius,          // 内半径（全额伤害）
				DamageOuterRadius,          // 外半径（无伤害）
				1.f,                        // 衰减指数
				UDamageType::StaticClass(), // 伤害类型
				TArray<AActor*>(),          // 忽略的Actor列表
				this,                       // 伤害来源
				FiringController            // 伤害控制者（用于计分）
			);
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
