#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * @brief 子弹投射物类 (ProjectileBullet)
 * 
 * 实弹武器（如手枪、步枪）发射的子弹
 * 
 * 特性：
 * - 使用 ProjectileMovementComponent 进行物理模拟
 * - 支持服务器倒带补偿
 * - 爆头判定（基于骨骼名称）
 * - 编辑器中可实时调整速度
 */
UCLASS()
class BATTLEFIELD_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()

public:
	/** 构造函数，创建投射物移动组件 */
	AProjectileBullet();

#if WITH_EDITOR
	/**
	 * @brief 编辑器属性变更回调
	 * 
	 * 在编辑器中修改 InitialSpeed 时，
	 * 自动同步更新 ProjectileMovementComponent 的速度
	 */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/**
	 * @brief 碰撞处理函数
	 * 
	 * 实现伤害逻辑：
	 * - 服务器直接计算伤害（非倒带模式）
	 * - 客户端请求服务器倒带验证（倒带模式）
	 * - 爆头判定（骨骼名称为 "head"）
	 */
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit) override;

	/** 组件开始播放时调用 */
	virtual void BeginPlay() override;
};
