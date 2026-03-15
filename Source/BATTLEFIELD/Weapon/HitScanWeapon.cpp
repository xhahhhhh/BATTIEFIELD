/**
 * @file HitScanWeapon.cpp
 * @brief 即时命中武器实现
 * 
 * 实现即时命中武器的射击逻辑：
 * - 射线检测立即命中
 * - 支持服务器倒带
 * - 爆头判定
 * - 特效生成
 */

#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"
#include "BATTLEFIELD/Components/LagCompensationComponent.h"
#include "BATTLEFIELD/PlayerController/BasePlayerController.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// 获取拥有者和控制器
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	// 获取枪口位置
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlashSocket");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		// 执行射线检测
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		// 处理命中角色
		ABaseCharacter* Character = Cast<ABaseCharacter>(FireHit.GetActor());
		if (Character && InstigatorController)
		{
			// 判断是否需要服务器权威伤害
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
			
			if (HasAuthority() && bCauseAuthDamage)
			{
				// 服务器直接应用伤害（爆头判定）
				const float DamageToCause = FireHit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;

				UGameplayStatics::ApplyDamage(
					Character,
					DamageToCause,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
			else if (!HasAuthority() && bUseServerSideRewind)
			{
				// 客户端请求服务器倒带验证
				OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(OwnerPawn) : OwnerCharacter;
				OwnerController = OwnerController == nullptr
					                  ? Cast<ABasePlayerController>(InstigatorController)
					                  : OwnerController;
				if (OwnerController && OwnerCharacter && OwnerCharacter->GetLagCompensation() && 
				    OwnerCharacter->IsLocallyControlled())
				{
					OwnerCharacter->GetLagCompensation()->ServerScoreRequest(
						OwnerCharacter,
						Start,
						HitTarget,
						OwnerController->GetServerTime() - OwnerController->SingleTripTime
					);
				}
			}
		}

		// 生成命中粒子
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
		
		// 播放命中音效
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}
		
		// 生成枪口火焰
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTransform
			);
		}
		
		// 播放射击音效
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	// 执行射线检测并生成光束效果
	UWorld* World = GetWorld();
	if (World)
	{
		// 计算射线终点（超出目标一定距离）
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		World->LineTraceSingleByChannel(OutHit, TraceStart, End, ECC_Visibility);
		
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		else
		{
			OutHit.ImpactPoint = End;
		}

		// 生成光束粒子
		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			if (Beam)
			{
				// 设置光束终点参数
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}
