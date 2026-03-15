/**
 * @file ProjectileGrenade.h
 * @brief 榴弹投射物类
 */

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileGrenade.generated.h"

/**
 * @brief 榴弹投射物类 (ProjectileGrenade)
 * 
 * 榴弹特点：
 * - 可弹跳（bShouldBounce = true）
 * - 弹跳时播放音效
 * - 定时爆炸（通过 DestroyTimer）
 * - 销毁时触发范围伤害
 */
UCLASS()
class BATTLEFIELD_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	/** 构造函数，创建榴弹网格和启用弹跳 */
	AProjectileGrenade();

	/** 销毁时触发爆炸伤害 */
	virtual void Destroyed() override;

protected:
	/** 
	 * @brief 组件开始播放时调用
	 * 
	 * 初始化：
	 * - 生成尾迹系统
	 * - 启动自动销毁定时器
	 * - 绑定弹跳事件
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief 弹跳回调函数
	 * @param ImpactResult 碰撞结果
	 * @param ImpactVelocity 碰撞时的速度
	 * 
	 * 榴弹弹跳时播放音效
	 */
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
	/** 弹跳时播放的音效 */
	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;
};
