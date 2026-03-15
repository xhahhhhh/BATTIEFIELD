
#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "ProjectileWeapon.generated.h"

class AProjectile;

/**
 * @brief 投射物武器 (ProjectileWeapon)
 * 
 * 发射实体投射物的武器：
 * - 子弹有飞行时间和轨迹
 * - 支持服务器倒带（不同客户端生成不同类型投射物）
 * - 火箭、榴弹等使用此类型
 */
UCLASS()
class BATTLEFIELD_API AProjectileWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief 开火
	 * @param HitTarget 目标位置
	 * 
	 * 根据网络情况生成不同类型的投射物
	 */
	virtual void Fire(const FVector& HitTarget) override;

private:
	/** 标准投射物类（服务器使用） */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ProjectileClass;

	/** 
	 * @brief 服务器倒带投射物类
	 * 
	 * 高延迟客户端使用的非复制投射物，用于本地预测
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;
};
