/**
 * @file ProjectileBullet.cpp
 * @brief 子弹投射物实现
 * 
 * 实现子弹的物理飞行、命中伤害计算和服务器倒带补偿
 */

#include "ProjectileBullet.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/LagCompensationComponent.h"
#include "BATTLEFIELD/PlayerController/BasePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	// 创建投射物移动组件
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ProjectileMovementComponent"));

	// 设置移动属性
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 获取变更的属性名
	FName PropertyName = PropertyChangedEvent.Property != nullptr
		                     ? PropertyChangedEvent.Property->GetFName()
		                     : NAME_None;

	// 如果修改了 InitialSpeed，同步更新移动组件
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	// 获取所有者角色和控制器
	ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		ABasePlayerController* OwnerController = Cast<ABasePlayerController>(OwnerCharacter->Controller);
		if (OwnerController)
		{
			// 服务器且不使用倒带：直接应用伤害
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				// 爆头判定：命中骨骼名称为 "head"
				const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;

				UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this,
				                            UDamageType::StaticClass());
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}

			// 倒带模式：客户端请求服务器验证
			ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() &&
				HitCharacter)
			{
				OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime
				);
			}
		}
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	// 这里可以放置投射物路径预测调试代码（已注释）
	/*
	FPredictProjectilePathParams PredictParams;
	PredictParams.bTraceWithChannel = true;
	PredictParams.bTraceWithCollision = true;
	PredictParams.DrawDebugTime = 5.f;
	PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PredictParams.LaunchVelocity = GetActorForwardVector() * 3500.f;
	PredictParams.MaxSimTime = 4.f;
	PredictParams.ProjectileRadius = 5.f;
	PredictParams.SimFrequency = 30.f;
	PredictParams.StartLocation = GetActorLocation();
	PredictParams.TraceChannel = ECC_Visibility;
	PredictParams.ActorsToIgnore.Add(this);

	FPredictProjectilePathResult PredictResult;

	UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);
	*/
}
