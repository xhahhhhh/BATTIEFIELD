
#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "HitScanWeapon.generated.h"

class UParticleSystem;
class USoundCue;

/**
 * @brief 即时命中武器 (HitScanWeapon)
 * 
 * 继承自武器基类，实现即时命中射击逻辑：
 * - 射线检测立即命中目标
 * - 支持服务器倒带补偿
 * - 爆头判定（head骨骼）
 * - 视觉效果（枪口火焰、弹道光束、命中粒子）
 * 
 * 适用于：步枪、冲锋枪、狙击枪等
 */
UCLASS()
class BATTLEFIELD_API AHitScanWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief 开火
	 * @param HitTarget 目标位置
	 * 
	 * 执行射线检测、伤害应用、特效生成
	 */
	virtual void Fire(const FVector& HitTarget) override;

protected:
	/**
	 * @brief 武器射线检测
	 * @param TraceStart 射线起点
	 * @param HitTarget 目标位置
	 * @param OutHit 输出检测结果
	 * 
	 * 从枪口发射射线检测，生成光束效果
	 */
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

	/** 命中粒子效果 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	/** 命中音效 */
	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;

private:
	/** 光束粒子效果 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	/** 枪口火焰粒子 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	/** 射击音效 */
	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;
};
