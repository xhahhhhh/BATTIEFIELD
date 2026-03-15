
#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * @brief 霰弹枪 (Shotgun)
 * 
 * 发射多个弹丸的散射武器：
 * - 一次射击发射多个弹丸（NumberOfPellets）
 * - 每个弹丸独立计算命中和伤害
 * - 支持爆头判定（head骨骼）
 * - 支持服务器倒带
 * 
 * 适用于近距离高伤害场景
 */
UCLASS()
class BATTLEFIELD_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	/**
	 * @brief 发射霰弹
	 * @param HitTargets 多个弹丸的目标位置数组
	 * 
	 * 对每个弹丸执行射线检测和伤害计算
	 */
	virtual void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);

	/**
	 * @brief 计算霰弹散射目标点
	 * @param HitTarget 准星目标位置
	 * @param HitTargets 输出的多个弹丸目标位置
	 * 
	 * 在目标周围球形范围内生成多个随机散射点
	 */
	void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets);

private:
	/** 弹丸数量 */
	UPROPERTY(EditAnywhere, Category = "WeaponScatter")
	uint32 NumberOfPellets = 10;
};
