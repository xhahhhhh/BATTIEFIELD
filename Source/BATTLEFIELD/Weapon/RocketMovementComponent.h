/**
 * @file RocketMovementComponent.h
 * @brief 火箭移动组件类
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RocketMovementComponent.generated.h"

/**
 * @brief 火箭移动组件 (RocketMovementComponent)
 * 
 * 自定义的投射物移动组件，专用于火箭弹
 * 
 * 主要修改：
 * - 覆盖 HandleBlockingHit 以改进碰撞处理
 * - 返回 AdvanceNextSubstep 确保碰撞后继续移动
 * - 防止火箭弹在碰撞时卡住或异常停止
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEFIELD_API URocketMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

protected:
	/**
	 * @brief 处理阻挡碰撞
	 * @param Hit 碰撞结果
	 * @param TimeTick 当前时间步长
	 * @param MoveDelta 移动增量
	 * @param SubTickTimeRemaining 剩余子步时间
	 * @return 碰撞处理结果
	 * 
	 * 覆盖此方法以返回 AdvanceNextSubstep，
	 * 确保火箭弹在碰撞后继续前进（触发爆炸）
	 */
	virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit, float TimeTick,
	                                                   const FVector& MoveDelta, float& SubTickTimeRemaining) override;

	/**
	 * @brief 处理碰撞影响
	 * @param Hit 碰撞结果
	 * @param TimeSlice 时间切片
	 * @param MoveDelta 移动增量
	 * 
	 * 调用父类实现处理碰撞
	 */
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f,
	                          const FVector& MoveDelta = FVector::ZeroVector) override;
};
