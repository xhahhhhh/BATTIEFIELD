/**
 * @file Shotgun.cpp
 * @brief 霰弹枪实现
 * 
 * 实现霰弹枪的射击逻辑：
 * - 多弹丸散射
 * - 独立命中检测
 * - 伤害累加（身体/爆头分开计算）
 * - 服务器倒带支持
 */

#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/LagCompensationComponent.h"
#include "BATTLEFIELD/PlayerController/BasePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	// 执行基础射击动画和弹壳生成（传入空向量表示不传入单个目标）
	AWeaponBase::Fire(FVector());
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	// 获取枪口位置
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlashSocket");
	if (MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		// 记录命中情况
		TMap<ABaseCharacter*, uint32> HitMap;           // 身体命中次数
		TMap<ABaseCharacter*, uint32> HeadShotHitMap;   // 爆头命中次数
		
		// 对每个弹丸执行射线检测
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABaseCharacter* Character = Cast<ABaseCharacter>(FireHit.GetActor());
			if (Character)
			{
				// 判断是否为爆头
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");
				if (bHeadShot)
				{
					if (HeadShotHitMap.Contains(Character))
						HeadShotHitMap[Character]++;
					else 
						HeadShotHitMap.Emplace(Character, 1);
				}
				else
				{
					if (HitMap.Contains(Character))
						HitMap[Character]++;
					else 
						HitMap.Emplace(Character, 1);
				}

				// 生成命中特效
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
						FireHit.ImpactPoint,
						.5f,
						FMath::RandRange(-0.5f, 0.5f)
					);
				}
			}
		}

		// 计算每个角色的总伤害
		TArray<ABaseCharacter*> HitCharacters;
		TMap<ABaseCharacter*, float> DamageMap;

		// 累加身体伤害
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key)
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);
				HitCharacters.AddUnique(HitPair.Key);
			}
		}
		
		// 累加爆头伤害
		for (auto HeadShotHitPair : HeadShotHitMap)
		{
			if (HeadShotHitPair.Key)
			{
				if (DamageMap.Contains(HeadShotHitPair.Key))
					DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage;
				else 
					DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotHitPair.Key);
			}
		}

		// 应用伤害（服务器权威）
		bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						DamagePair.Key,
						DamagePair.Value,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}

		// 客户端请求服务器倒带验证
		if (!HasAuthority() && bUseServerSideRewind)
		{
			OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABaseCharacter>(OwnerPawn) : OwnerCharacter;
			OwnerController = OwnerController == nullptr
				                  ? Cast<ABasePlayerController>(InstigatorController)
				                  : OwnerController;
			if (OwnerController && OwnerCharacter && OwnerCharacter->GetLagCompensation() && 
			    OwnerCharacter->IsLocallyControlled())
			{
				OwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					Start,
					HitTargets,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime
				);
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	// 计算多个弹丸的散射目标点
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlashSocket");
	if (MuzzleFlashSocket == nullptr) return;
	
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	// 生成多个随机散射点
	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
		HitTargets.Emplace(ToEndLoc);
	}
}
