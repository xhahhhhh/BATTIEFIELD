/**
 * @file ProjectileRocket.h
 * @brief 火箭弹投射物类
 */

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * @brief 火箭弹投射物类 (ProjectileRocket)
 * 
 * 火箭筒发射的火箭弹，特点：
 * - 使用自定义的 RocketMovementComponent 实现平滑移动
 * - 命中时产生范围爆炸伤害
 * - 飞行时播放循环音效
 * - 命中后延迟销毁以播放特效
 */
UCLASS()
class BATTLEFIELD_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	/** 构造函数，创建火箭网格和自定义移动组件 */
	AProjectileRocket();

	/** 销毁时调用 */
	virtual void Destroyed() override;

protected:
	/**
	 * @brief 碰撞处理函数
	 * 
	 * 实现：
	 * - 忽略自伤（不伤害发射者自身）
	 * - 触发范围伤害
	 * - 隐藏网格、禁用碰撞、停止音效
	 * - 延迟销毁以播放命中特效
	 */
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit) override;

	/** 
	 * @brief 组件开始播放时调用
	 * 
	 * 初始化：
	 * - 客户端绑定碰撞事件
	 * - 生成尾迹系统
	 * - 播放飞行循环音效
	 */
	virtual void BeginPlay() override;

	/** 飞行循环音效 */
	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	/** 飞行音效组件实例 */
	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	/** 音效衰减设置 */
	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;

	/** 自定义火箭移动组件 */
	UPROPERTY(VisibleAnywhere)
	class URocketMovementComponent* RocketMovementComponent;
};
